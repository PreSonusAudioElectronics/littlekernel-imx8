

#include "ivshmem-pipe.h"
#include "ivshmem-endpoint.h"
#include "ivshmem-serial.h"
#include <debug.h>
#include <assert.h>
#include <stdio.h>
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

struct ivshm_serial_service {
    struct list_node node;
    unsigned id;
    struct ivshm_endpoint *ept;
    mutex_t tx_lock;
    cbuf_t tx_cbuf;
    thread_t *thread;
    bool running;
    char *tx_buf;
};

#define IVSHM_MAX_CBUF_SIZE (1024 * 64)

#define IVSHM_SERIAL_TX_BUF_SIZE (64)

static struct list_node serial_service_list =
        LIST_INITIAL_VALUE(serial_service_list);

/*!
 * \brief and event for signalling when the ivshmem is ready to run
 */
static event_t ivshm_ready_evt = EVENT_INITIAL_VALUE(ivshm_ready_evt, 0, 0);


static void ivshm_start_serial(struct ivshm_endpoint *);

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

    printf("payload : %lu bytes strlen %lu, content: %s",
          len, strnlen(payload, len), payload);

    for(unsigned i=0; i<len; ++i)
    {
        ivshm_putchar(ep->id, payload[i]);
    }
    return 0;
}

int ivshm_init_serial(struct ivshm_info *info)
{
    event_signal(&ivshm_ready_evt, false);

    // ep_serial = ivshm_endpoint_create(
    //              "ivshm_serial",
    //              IVSHM_EP_ID_SERIAL,
    //              ivshm_serial_consume,
    //              info,
    //              8 * 1024,
    //              0
    //          );

    // DEBUG_ASSERT(NULL != ep_serial );

    // ivshm_start_serial(ep_serial);

    return 0;
}

void ivshm_exit_serial(struct ivshm_info *info)
{
    // ivshm_stop_serial(ep_serial);
    // ivshm_endpoint_destroy(ep_serial);
    struct ivshm_serial_service *service, *next;
    int ret = 0;

    list_for_every_entry_safe(&serial_service_list, service, next,
        struct ivshm_serial_service, node) {
            service->running = false;
            thread_join(service->thread, &ret, 5000);
            // mutex_destroy(&service->tx_lock);
            free(service->tx_cbuf.buf);
            ivshm_endpoint_destroy(service->ept);
            list_delete(&service->node);
            free(service);
        }
}

static inline struct ivshm_serial_service
                        *_ivshm_serial_get_service(unsigned id)
{
    struct ivshm_serial_service *service;

    list_for_every_entry(&serial_service_list, service,
                         struct ivshm_serial_service, node) 
    {
        if (service->id == id)
            return service;
    }

    printlk(LK_ERR, "%s:%d: Could not find serial-%d endpoint\n",
            __PRETTY_FUNCTION__, __LINE__, id);
    return NULL;
}

int ivshm_putchar(unsigned id, char c)
{
    struct ivshm_serial_service *service;
    int ret = 0;

    service = _ivshm_serial_get_service(id);
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

static void ivshm_start_serial(struct ivshm_endpoint *ep)
{
    

    // struct ivshm_serial *con = _con;

    // con->thread = thread_create(
    //                 "ivshm_serial_thread",
    //                 ivshm_serial_thread,
    //                 (void *) con,
    //                 LOW_PRIORITY - 1,
    //                 IVSHM_LOGGER_BUF_SIZE +  4096
    //              );

    // con->running = true;
    // con->ep = ep;
    // smp_wmb();
    // thread_resume(con->thread);
}

// static void ivshm_stop_serial(struct ivshm_endpoint *ep)
// {
//     struct ivshm_serial *con = _con;
//     int retcode;

//     con->running = false;
//     smp_wmb();
//     thread_join(con->thread, &retcode, 1000);
// }

static void ivshm_hook_serial_init(unsigned level)
{
    // memset(_con, 0x0, sizeof(*_con));

    // cbuf_initialize_etc(&_con->tx_cbuf, IVSHM_SERIAL_BUFFER_SIZE, &ivshm_serial_buf);

    // mutex_init(&_con->tx_lock);
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

    service->id = id;
    snprintf(name, IVSHM_EP_MAX_NAME, "serial-%u", service->id);
    service->ept = ivshm_endpoint_create(
            name,
            service->id,
            ivshm_serial_consume,
            info,
            ep_size,
            0 // fixme
        );

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

    // initialize tx buffer
    service->tx_buf = malloc(IVSHM_SERIAL_TX_BUF_SIZE);
    if(!service->tx_buf) {
        ret = ERR_NO_MEMORY;
        goto cleanup;
    }

    service->running = true;
    thread_resume(service->thread);
    list_add_tail(&serial_service_list, &service->node);

    return ret;

cleanup:
    if(service->tx_buf) { free(service->tx_buf); }
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

// LK_INIT_HOOK(ivshm_serial, ivshm_hook_serial_init, LK_INIT_LEVEL_PLATFORM_EARLY - 1);
DRIVER_EXPORT_WITH_LVL(ivshm_serial, &the_ops, DRIVER_INIT_CORE);


