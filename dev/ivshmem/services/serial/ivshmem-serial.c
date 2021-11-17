

#include "ivshmem-pipe.h"
#include "ivshmem-endpoint.h"
#include "ivshmem-serial.h"
#include <debug.h>
#include <assert.h>
#include <stdio.h>
#include <lib/cbuf.h>
#include <lk/init.h>
#include <lib/io.h>
#include <string.h>
#include <kernel/mutex.h>
#include <lib/console.h>
#include <string.h>

struct ivshm_serial {
    mutex_t tx_lock;
    cbuf_t tx_cbuf;
    thread_t *thread;
    bool running;
    struct ivshm_endpoint *ep;
};

#ifndef IVSHM_SERIAL_BUFFER_SIZE
#define IVSHM_SERIAL_BUFFER_SIZE (16 * 1024)
#endif
#ifndef IVSHM_LOGGER_BUF_SIZE
#define IVSHM_LOGGER_BUF_SIZE 4096
#endif

static struct ivshm_endpoint *ep_serial;
static char ivshm_serial_buf[IVSHM_SERIAL_BUFFER_SIZE];

#define IVSHM_SERIAL_TX_BUF_SIZE (64)
static char txbuf[IVSHM_SERIAL_TX_BUF_SIZE];

static struct ivshm_serial _ivshm_serial;
static struct ivshm_serial *_con = &_ivshm_serial;

#define MIN(x, y) ((x < y) ? x : y)

static void ivshm_start_serial(struct ivshm_endpoint *);
static void ivshm_stop_serial(struct ivshm_endpoint *);

static inline void write_chars(struct ivshm_endpoint *ep, char *buf, unsigned len)
{
    ASSERT( len <= IVSHM_SERIAL_TX_BUF_SIZE);
    struct ivshm_ep_buf ep_buf;
    memcpy(txbuf, buf, len);
    ivshm_ep_buf_init(&ep_buf);
    ivshm_ep_buf_add(&ep_buf, txbuf, len);
    ivshm_endpoint_write(ep, &ep_buf);
}

static ssize_t ivshm_serial_consume(struct ivshm_endpoint *ep, struct ivshm_pkt *pkt)
{
    char *payload = (char *) &pkt->payload;
    size_t len = ivshm_pkt_get_payload_length(pkt);

    printf("payload : %lu bytes strlen %lu, content: %s",
          len, strnlen(payload, len), payload);

    // echo back
    for(unsigned i=0; i<len; ++i)
    {
        ivshm_putchar(payload[i]);
    }
    return 0;
}

int ivshm_init_serial(struct ivshm_info *info)
{
    ep_serial = ivshm_endpoint_create(
                 "ivshm_serial",
                 IVSHM_EP_ID_SERIAL,
                 ivshm_serial_consume,
                 info,
                 8 * 1024,
                 0
             );

    DEBUG_ASSERT(NULL != ep_serial );

    ivshm_start_serial(ep_serial);

    return 0;
}

void ivshm_exit_serial(struct ivshm_info *info)
{
    ivshm_stop_serial(ep_serial);
    ivshm_endpoint_destroy(ep_serial);
}

inline int ivshm_putchar(char c)
{
    // If buffer is empty, send directly
    size_t used = cbuf_space_used(&_con->tx_cbuf);
    if( 0 == used )
    {
        write_chars(ep_serial, &c, 1);
        return 1;
    }

    // Otherwise, if buffer is full return ERR and drop char
    size_t avail = cbuf_space_avail(&_con->tx_cbuf);
    if( 0 == avail )
    {
        return -1;
    }

    // Otherwise, put char onto buff
    return cbuf_write_char(&_con->tx_cbuf, c, false);
}

inline int ivshm_getchar_noblock(char *out)
{
    return 0;
}

void ivshm_acquire_lock(void)
{
    mutex_acquire(&_con->tx_lock);
}

void ivshm_release_lock(void)
{
    mutex_release(&_con->tx_lock);
}

static int ivshm_serial_thread(void *arg)
{

    struct ivshm_serial *con = arg;
    struct ivshm_ep_buf ep_buf;
    iovec_t regions[2];
    size_t payload_sz;

    while (con->running) 
    {      
        thread_yield();
        ivshm_ep_buf_init(&ep_buf);
        event_wait(&con->tx_cbuf.event);
        payload_sz = cbuf_peek(&con->tx_cbuf, regions);
        DEBUG_ASSERT(payload_sz > 0);
        ivshm_ep_buf_add(&ep_buf, regions[0].iov_base, regions[0].iov_len);
        if (regions[1].iov_len)
            ivshm_ep_buf_add(&ep_buf, regions[1].iov_base, regions[1].iov_len);

        ivshm_endpoint_write(con->ep, &ep_buf);

        cbuf_read(&con->tx_cbuf, NULL, payload_sz, false);
    }

    return 0;
}

static void ivshm_start_serial(struct ivshm_endpoint *ep)
{

    struct ivshm_serial *con = _con;

    con->thread = thread_create(
                    "ivshm_serial_thread",
                    ivshm_serial_thread,
                    (void *) con,
                    LOW_PRIORITY - 1,
                    IVSHM_LOGGER_BUF_SIZE +  4096
                 );

    con->running = true;
    con->ep = ep;
    smp_wmb();
    thread_resume(con->thread);
}

static void ivshm_stop_serial(struct ivshm_endpoint *ep)
{
    struct ivshm_serial *con = _con;
    int retcode;

    con->running = false;
    smp_wmb();
    thread_join(con->thread, &retcode, 1000);
}

static void ivshm_hook_serial_init(unsigned level)
{
    memset(_con, 0x0, sizeof(*_con));

    cbuf_initialize_etc(&_con->tx_cbuf, IVSHM_SERIAL_BUFFER_SIZE, &ivshm_serial_buf);

    mutex_init(&_con->tx_lock);
}

LK_INIT_HOOK(ivshm_serial, ivshm_hook_serial_init, LK_INIT_LEVEL_PLATFORM_EARLY - 1);


