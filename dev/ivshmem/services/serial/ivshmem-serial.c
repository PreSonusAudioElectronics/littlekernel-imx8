

#include "ivshmem-pipe.h"
#include "ivshmem-endpoint.h"
#include "ivshmem-serial.h"
#include <debug.h>
#include <assert.h>
#include <stdio.h>
#include <trace.h>
#include <lib/cbuf.h>
#include <lk/init.h>
#include <lib/io.h>
#include <lib/appargs.h>
#include <string.h>
#include <kernel/mutex.h>
#include <lib/console.h>
#include <dev/driver.h>
#include <dev/ivshm.h>
#include <err.h>

#include <string.h>
#include <stdlib.h>

/******************************************************************************/
// Defines
/******************************************************************************/
#define IVSHM_MAX_CBUF_SIZE (1024 * 64)
#define IVSHM_SERIAL_TX_BUF_SIZE (64)
#define IVSHM_SERIAL_SERVICENAME_MAX_SIZE (128)

/******************************************************************************/
// Types
/******************************************************************************/
struct ivshm_serial_service {
    struct list_node node;
    unsigned id;
    struct ivshm_endpoint *ept;
    char const *name;
    mutex_t tx_lock;
    cbuf_t tx_cbuf;
    cbuf_t rx_cbuf;
    mutex_t rx_cbuf_lock;
    thread_t *thread;
    bool running;
    char *tx_buf;
    ivshm_serial_rx_cb_t rx_cb;
    bool buffer_rx;
};

/******************************************************************************/
// File-scope Globals
/******************************************************************************/
static struct list_node serial_service_list =
        LIST_INITIAL_VALUE(serial_service_list);

/*!
 * \brief and event for signalling when the ivshmem is ready to run
 */
static event_t ivshm_ready_evt = EVENT_INITIAL_VALUE(ivshm_ready_evt, 0, 0);

/******************************************************************************/
// Forward Declarations
/******************************************************************************/
static inline struct ivshm_serial_service
                        *_get_service(unsigned id);
static inline void write_chars(struct ivshm_serial_service *service, char *buf, unsigned len);

/******************************************************************************/
// Public Functions
/******************************************************************************/

int ivshm_init_serial(struct ivshm_info *info)
{
    event_signal(&ivshm_ready_evt, false);
    return 0;
}

void ivshm_exit_serial(struct ivshm_info *info)
{
    struct ivshm_serial_service *service, *next;
    int ret = 0;

    list_for_every_entry_safe(&serial_service_list, service, next,
        struct ivshm_serial_service, node) {
            service->running = false;
            thread_join(service->thread, &ret, 5000);
            mutex_destroy(&service->tx_lock);
            free(service->tx_cbuf.buf);
            ivshm_endpoint_destroy(service->ept);
            list_delete(&service->node);
            free(service);
        }
}

int ivshm_serial_putchar(unsigned id, char c)
{
    struct ivshm_serial_service *service;
    int ret = 0;

    service = _get_service(id);
    if(!service) {
        return ERR_NOT_FOUND;
    }

    // get the tx_cbuf for this id
    cbuf_t *cbuf = &service->tx_cbuf;

    // get the lock
    mutex_acquire(&service->tx_lock);

    // If buffer is empty, send directly
    size_t used = cbuf_space_used(cbuf);
    if( 0 == used )
    {
        write_chars(service, &c, 1);
        ret = 1;
        goto unlock;
    }

    // Otherwise, if buffer is full return ERR and drop char
    size_t avail = cbuf_space_avail(cbuf);
    if( 0 == avail )
    {
        ret = -1;
        goto unlock;
    }

    // Otherwise, put char onto buff
    ret = cbuf_write_char(cbuf, c, false);

unlock:
    mutex_release(&service->tx_lock);
    return ret;
}

inline int ivshm_getchar_noblock(char *out)
{
    return 0;
}

int ivshm_serial_register_rx_cb(unsigned id, ivshm_serial_rx_cb_t cb)
{
    if(!cb){
        return ERR_INVALID_ARGS;
    }

    struct ivshm_serial_service *service = _get_service(id);
    if( !service ) {
        return ERR_NOT_FOUND;
    }

    service->rx_cb = cb;
    service->buffer_rx = false;

    // if there is data in our rx buffer, send it to the callback now
    mutex_acquire(&service->rx_cbuf_lock);
    size_t nchars_buffered = cbuf_space_used(&service->rx_cbuf);
    char tmpbuf[IVSHM_SERIAL_RX_CB_MAX_LEN];
    while( nchars_buffered > 0 ) 
    {
        size_t n_2_read;
        if( nchars_buffered > IVSHM_SERIAL_RX_CB_MAX_LEN ) 
        {
            n_2_read = IVSHM_SERIAL_RX_CB_MAX_LEN;
        } else 
        {
            n_2_read = nchars_buffered;
        }
        size_t n_read = cbuf_read(&service->rx_cbuf, tmpbuf, n_2_read, true);
        
        cb(id, tmpbuf, n_read);

        nchars_buffered -= n_read;
    }

    mutex_release(&service->rx_cbuf_lock);

    return NO_ERROR;
}

int ivshm_serial_getchars_noblock(unsigned id, char *buf, uint16_t buflen)
{
    int ret = 0;

    if(!buf) {
        return ERR_NO_MEMORY;
    }

    struct ivshm_serial_service *service = _get_service(id);
    if(!service) {
        return ERR_NOT_FOUND;
    }

    mutex_acquire(&service->rx_cbuf_lock);
    size_t n_2_read = cbuf_space_used(&service->rx_cbuf);
    if( n_2_read > 0 ){
        if( n_2_read > buflen ) {
            n_2_read = buflen;
        }

        ret = cbuf_read(&service->rx_cbuf, buf, n_2_read, false);
    } else {
        ret = 0;
    }
    mutex_release(&service->rx_cbuf_lock);

    return ret;
}

/******************************************************************************/
// Private Functions
/******************************************************************************/
static inline void write_chars(struct ivshm_serial_service *service, char *buf, unsigned len)
{
    ASSERT( len <= IVSHM_SERIAL_TX_BUF_SIZE);
    struct ivshm_ep_buf ep_buf;
    memcpy(service->tx_buf, buf, len);
    ivshm_ep_buf_init(&ep_buf);
    ivshm_ep_buf_add(&ep_buf, service->tx_buf, len);
    ivshm_endpoint_write(service->ept, &ep_buf);
}

static ssize_t ivshm_serial_consume(struct ivshm_endpoint *ep, struct ivshm_pkt *pkt)
{
    char *payload = (char *) &pkt->payload;
    size_t len = ivshm_pkt_get_payload_length(pkt);
    unsigned id = IVSHM_EP_GET_ID(ep->id);

    TRACEF("recvd %lu bytes on ept %d, content: '%s' \n", len, id, payload);

    // get the service struct and either buffer or send it to callback
    struct ivshm_serial_service *service = _get_service(id);
    if( service->buffer_rx )
    {
        // service in buffered mode so put data into the buffer
        mutex_acquire(&service->rx_cbuf_lock);
        size_t n_buffered = cbuf_write(&service->rx_cbuf, payload, len, true);
        mutex_release(&service->rx_cbuf_lock);
        DEBUG_ASSERT( n_buffered == len );
    }
    else
    {
        // we're in callback mode
        DEBUG_ASSERT( NULL != service->rx_cb );
        service->rx_cb(id, payload, len);
    }
    return 0;
}

static inline struct ivshm_serial_service
                        *_get_service(unsigned id)
{
    struct ivshm_serial_service *service;

    list_for_every_entry(&serial_service_list, service,
                         struct ivshm_serial_service, node) 
    {
        if (service->id == id)
            return service;
    }

    printlk(LK_ERR, "%s:%d: Could not find serial_%u endpoint\n",
            __PRETTY_FUNCTION__, __LINE__, id);
    return NULL;
}

static int ivshm_serial_tx_thread(void *arg)
{
    struct ivshm_serial_service *service = arg;
    struct ivshm_ep_buf ep_buf;
    iovec_t regions[2];
    size_t payload_sz;

    while (service->running) 
    {      
        ivshm_ep_buf_init(&ep_buf);
        event_wait(&service->tx_cbuf.event);
        payload_sz = cbuf_peek(&service->tx_cbuf, regions);
        DEBUG_ASSERT(payload_sz > 0);
        ivshm_ep_buf_add(&ep_buf, regions[0].iov_base, regions[0].iov_len);
        if (regions[1].iov_len)
            ivshm_ep_buf_add(&ep_buf, regions[1].iov_base, regions[1].iov_len);

        ivshm_endpoint_write(service->ept, &ep_buf);

        cbuf_read(&service->tx_cbuf, NULL, payload_sz, false);
        thread_yield();
    }

    return 0;
}

static inline void print_err_property(char *property_name)
{
    printlk(LK_ERR, "%s:%d: Endpoint %s property cannot be read, aborting!\n",
        __PRETTY_FUNCTION__, __LINE__, property_name);
}

static status_t ivshm_serial_dev_init(struct device *dev)
{
    int ret = NO_ERROR;
    char *property_name = NULL;
    char * cbuf_mem = NULL;
    char name[IVSHM_EP_MAX_NAME];
    uint32_t id, ep_size;
    struct ivshm_serial_service *service = NULL;

    struct ivshm_dev_data *ivshm_dev = ivshm_get_device(0);
    if(!ivshm_dev) {
        return ERR_NOT_READY;
    }

    struct ivshm_info *info = ivshm_dev->handler_arg;
    if (!info) {
        return ERR_NOT_READY;
    }

    property_name = (char*)"id";
    ret = of_device_get_int32(dev, property_name, &id);
    if(ret) {
        print_err_property(property_name);
        return ERR_NOT_FOUND;
    }

    property_name = (char*)"size";
    ret = of_device_get_int32(dev, property_name, &ep_size);
    if(ret) {
        print_err_property(property_name);
        return ERR_NOT_FOUND;
    }

    DEBUG_ASSERT(ep_size < IVSHM_MAX_CBUF_SIZE);

    service = malloc(sizeof(struct ivshm_serial_service));
    if(!service) {
        ret = ERR_NO_MEMORY;
        goto cleanup;
    }
    
    memset(service, 0, sizeof(struct ivshm_serial_service));

    property_name = (char*)"id_str";
    ret = of_device_get_strings(dev, property_name, &service->name, 1);
    if( 1 != ret ) {
        TRACEF("Failed to get id_str!\n");
        ret = ERR_NOT_FOUND;
        goto cleanup;
    }

    service->id = id;
    snprintf(name, IVSHM_EP_MAX_NAME, "serial_%u_%s", service->id, service->name);
    service->ept = ivshm_endpoint_create(
            name,
            service->id,
            ivshm_serial_consume,
            info,
            ep_size,
            0 // fixme
        );
    
    if(!service->ept) {
        ret = ERR_NO_MEMORY;
        goto cleanup;
    }

    printf("\n%s:%u: new endpoint id = %u\n", __PRETTY_FUNCTION__, __LINE__,
        IVSHM_EP_GET_ID(service->ept->id) );

    // Setup tx thread to transmit queued buffers
    char const * prefix = "ivshm-serial";
    char name_buf[32];
    size_t len = strnlen(prefix, 32);
    memcpy(name_buf, prefix, len);
    snprintf(name_buf + len, 32-len, "%u", id);
    service->thread = thread_create(
        name_buf,
        ivshm_serial_tx_thread,
        (void*)service,
        IVSHM_EP_GET_PRIO(service->ept->id),
        DEFAULT_STACK_SIZE );

    // initialize tx circ buffer
    cbuf_mem = malloc(ep_size);
    if(!cbuf_mem) {
        ret = ERR_NO_MEMORY;
        goto cleanup;
    }
    cbuf_initialize_etc(&service->tx_cbuf, ep_size, cbuf_mem);

    // initialize the rx circ buffer
    cbuf_mem = NULL;
    cbuf_mem = malloc(ep_size);
    if(!cbuf_mem) {
        ret = ERR_NO_MEMORY;
        goto cleanup;
    }
    cbuf_initialize_etc(&service->rx_cbuf, ep_size, cbuf_mem);

    // initialize tx buffer
    service->tx_buf = malloc(IVSHM_SERIAL_TX_BUF_SIZE);
    if(!service->tx_buf) {
        ret = ERR_NO_MEMORY;
        goto cleanup;
    }

    mutex_init(&service->tx_lock);
    mutex_init(&service->rx_cbuf_lock);
    service->buffer_rx = true;
    service->running = true;
    thread_resume(service->thread);
    list_add_tail(&serial_service_list, &service->node);

    return ret;

cleanup:
    if(service->tx_buf) { free(service->tx_buf); }
    if(service->rx_cbuf.buf) { free(service->rx_cbuf.buf); }
    if(service->tx_cbuf.buf) { free(service->tx_cbuf.buf); }
    if(service->ept) { ivshm_endpoint_destroy(service->ept); }
    if(service->thread) {
        thread_join(service->thread, NULL, 5000);
        free(service->thread);
    }
    if(service) { free(service); }

    return ret;
}

static struct driver_ops the_ops = {
    .init = ivshm_serial_dev_init
};

DRIVER_EXPORT_WITH_LVL(ivshm_serial, &the_ops, DRIVER_INIT_CORE);
