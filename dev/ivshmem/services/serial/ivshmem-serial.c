

#include "ivshmem-pipe.h"
#include "ivshmem-endpoint.h"
#include "ivshmem-serial.h"
#include <debug.h>
// #include <lib/klog.h>
#include <stdio.h>

static struct ivshm_endpoint *ep_serial;

#define MIN(x, y) ((x < y) ? x : y)

#include <lib/console.h>
#include <string.h>
static void ivshm_start_logger(struct ivshm_endpoint *);
static void ivshm_stop_logger(struct ivshm_endpoint *);

static ssize_t ivshm_serial_consume(struct ivshm_endpoint *ep, struct ivshm_pkt *pkt)
{
    char *payload = (char *) &pkt->payload;
    size_t len = ivshm_pkt_get_payload_length(pkt);

    printf("payload : %lu bytes strlen %lu, content: %s",
          len, strnlen(payload, len), payload);

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

    ivshm_start_logger(ep_serial);

    return 0;
}

void ivshm_exit_serial(struct ivshm_info *info)
{
    ivshm_stop_logger(ep_serial);
    ivshm_endpoint_destroy(ep_serial);
}

#include <lk/init.h>
#include <lib/cbuf.h>
#include <lib/io.h>
#include <string.h>
#include "assert.h"

struct ivshm_serial {
    print_callback_t print_cb;
    spin_lock_t tx_lock;
    cbuf_t tx_buf;
    thread_t *thread;
    bool running;
    struct ivshm_endpoint *ep;
};

#ifndef IVSHM_CONSOLE_BUFFER_SIZE
#define IVSHM_CONSOLE_BUFFER_SIZE (16 * 1024)
#endif
#ifndef IVSHM_LOGGER_BUF_SIZE
#define IVSHM_LOGGER_BUF_SIZE 4096
#endif


static char ivshm_serial_buf[IVSHM_CONSOLE_BUFFER_SIZE];

static struct ivshm_serial _ivshm_serial;
static struct ivshm_serial *_con = &_ivshm_serial;

static int ivshm_serial_thread(void *arg)
{

    struct ivshm_serial *con = arg;
    struct ivshm_ep_buf ep_buf;
    iovec_t regions[2];
    size_t payload_sz;

    while (con->running) {
        ivshm_ep_buf_init(&ep_buf);
        event_wait(&con->tx_buf.event);
        payload_sz = cbuf_peek(&con->tx_buf, regions);
        DEBUG_ASSERT(payload_sz > 0);
        ivshm_ep_buf_add(&ep_buf, regions[0].iov_base, regions[0].iov_len);
        if (regions[1].iov_len)
            ivshm_ep_buf_add(&ep_buf, regions[1].iov_base, regions[1].iov_len);

        ivshm_endpoint_write(con->ep, &ep_buf);

        cbuf_read(&con->tx_buf, NULL, payload_sz, false);
    }

    return 0;
}


static void ivshm_start_logger(struct ivshm_endpoint *ep)
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

static void ivshm_stop_logger(struct ivshm_endpoint *ep)
{
    struct ivshm_serial *con = _con;
    int retcode;

    con->running = false;
    smp_wmb();
    thread_join(con->thread, &retcode, 1000);
}

static void ivshm_serial_print(print_callback_t *cb, const char *str, size_t len)
{
    struct ivshm_serial *con = cb->context;
    cbuf_write(&con->tx_buf, str, len, false);
}

static void ivshm_hook_serial_init(unsigned level)
{
    memset(_con, 0x0, sizeof(*_con));

    cbuf_initialize_etc(&_con->tx_buf, IVSHM_CONSOLE_BUFFER_SIZE, &ivshm_serial_buf);

    spin_lock_init(&_con->tx_lock);

    _con->print_cb.print = ivshm_serial_print;
    _con->print_cb.context = _con;

    register_print_callback(&_con->print_cb);
}

LK_INIT_HOOK(ivshm_serial, ivshm_hook_serial_init, LK_INIT_LEVEL_PLATFORM_EARLY - 1);

