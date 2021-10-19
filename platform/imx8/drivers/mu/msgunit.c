#include <stdlib.h>

#include <sys/types.h>
#include <lib/appargs.h>
#include <debug.h>
#include <trace.h>
#include <kernel/spinlock.h>
#include <kernel/mutex.h>
#include <delay.h>
#include <err.h>
#include <arch/arch_ops.h>
#include <list.h>

#include <dev/class/msgunit.h>
#include <dev/interrupt.h>
#include "fsl_mu.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define LOCAL_TRACE 1

struct imx_msgunit_state {
    int bus_id;
    MU_Type *io_base;
    mutex_t mutex;
    struct device *device;
    msgunit_tx_cb_t tx_callbacks[MU_TR_COUNT];
    msgunit_rx_cb_t rx_callbacks[MU_RR_COUNT];
    struct list_node node;
};

/*******************************************************************************
 * Variables
 ******************************************************************************/


/*******************************************************************************
 * Forward Declarations
 ******************************************************************************/
static enum handler_return msgunit_isr (void *);

/*******************************************************************************
 * Code
 ******************************************************************************/

static inline struct imx_msgunit_state *get_state(struct device *dev)
{
    return (struct imx_msgunit_state*)dev->state;
}

static inline MU_Type *get_base(struct device *dev)
{
    struct imx_msgunit_state *state = get_state(dev);
    ASSERT(state);
    return state->io_base;
}

static inline struct msgunit_ops *get_ops(struct device *dev)
{
    return device_get_driver_ops(dev, struct msgunit_ops, std);
}


static struct list_node imx_msgunit_list = LIST_INITIAL_VALUE(imx_msgunit_list);
static spin_lock_t imx_msgunit_list_lock = SPIN_LOCK_INITIAL_VALUE;

static status_t imx_msgunit_init(struct device *dev)
{
    TRACEF("called with dev: %p\n", dev);

    const struct device_config_data *config = dev->config;
    struct imx_msgunit_state *state;
    status_t status = NO_ERROR;

    state = malloc(sizeof(struct imx_msgunit_state));
    ASSERT(state);
    memset(state, 0, sizeof(*state));
    dev->state = state;
    state->device = dev;

    state->bus_id = config->bus_id;
    TRACEF("Initializing Msg Unit with bus-id: %d\n", state->bus_id );

    mutex_init(&state->mutex);

    struct device_cfg_reg *reg =
                    device_config_get_register_by_name(config, "core");
    ASSERT(reg);
    struct device_cfg_irq *irq =
                        device_config_get_irq_by_name(config, "core");
    ASSERT(irq);

    TRACEF("MU register base = %p\n", (void*)reg->base );
    TRACEF("MU register vbase = %p\n", (void*)reg->vbase );

    TRACEF("Will now try to access the MU registers...\n");
    udelay(100000);
    state->io_base = (MU_Type*)reg->vbase;

    uint32_t sr = state->io_base->SR;
    TRACEF("MU CR = %x\n", sr);

    TRACEF("Registering MU ISR to interrupt vector %d..\n", irq->irq );
    status = register_int_handler(irq->irq, msgunit_isr, dev);
    ASSERT( NO_ERROR == status );    
    
    TRACEF("Enabling MU Interrupt for RX Channel 3..\n");
    MU_EnableInterrupts(state->io_base, kMU_Rx3FullInterruptEnable );

    TRACEF("Unmasking MU Interrupt on GIC..\n");
    status = unmask_interrupt( irq->irq );
    ASSERT( NO_ERROR == status );

    spin_lock_saved_state_t lock_state;
    spin_lock_irqsave(&imx_msgunit_list_lock, lock_state);

    list_add_tail(&imx_msgunit_list, &state->node );

    spin_unlock_irqrestore(&imx_msgunit_list_lock, lock_state );

    TRACE_EXIT;

    return 0;
}

static status_t msgunit_send_msg(struct device *dev, uint32_t reg_idx, uint32_t msg)
{
    MU_SendMsg( get_base(dev), reg_idx, msg );
    return NO_ERROR;
}

static status_t msgunit_receive_msg(struct device *dev, uint32_t reg_idx, uint32_t *dst)
{
    ASSERT(dst);
    *dst = MU_ReceiveMsg( get_base(dev), reg_idx);
    return NO_ERROR;
}

static status_t msgunit_register_tx_callback(struct device *dev, uint32_t idx, msgunit_tx_cb_t cb)
{
    TRACE_ENTRY;

    if( idx >= MU_TR_COUNT ) {
        TRACE_EXIT;
        return ERR_NOT_FOUND;
    }

    if( get_state(dev)->tx_callbacks[idx] != NULL ) {
        TRACE_EXIT;
        return ERR_ALREADY_BOUND;
    }

    get_state(dev)->tx_callbacks[idx] = cb;
    TRACE_EXIT;
    return NO_ERROR;
}

static status_t msgunit_register_rx_callback(struct device *dev, uint32_t idx, msgunit_rx_cb_t cb)
{
    if( idx >= MU_TR_COUNT ) {
        TRACE_EXIT;
        return ERR_NOT_FOUND;
    }

    if( get_state(dev)->rx_callbacks[idx] != NULL ) {
        TRACE_EXIT;
        return ERR_ALREADY_BOUND;
    }

    get_state(dev)->rx_callbacks[idx] = cb;
    TRACE_EXIT;
    return NO_ERROR;
}

static struct device_class msgunit_device_class = {
    .name = "msgunit",
};

static struct msgunit_ops imx_msgunit_ops = {
    .std = {
        .device_class = & msgunit_device_class,
        .init = imx_msgunit_init,
    },
    .register_tx_callback = msgunit_register_tx_callback,
    .register_rx_callback = msgunit_register_rx_callback,
    .send_msg = msgunit_send_msg,
};

DRIVER_EXPORT_WITH_LVL(msgunit, &imx_msgunit_ops.std, DRIVER_INIT_PLATFORM);

static inline void invoke_tx_cb(msgunit_tx_cb_t *array, uint32_t idx)
{
    if( array[idx] ) {
        array[idx]();
    }
}

static inline void invoke_rx_cb(msgunit_rx_cb_t *array, uint32_t idx, uint32_t msg)
{
    if( array[idx] ) {
        array[idx](msg);
    }
}

static enum handler_return msgunit_isr (void *args)
{
    // figure out what kind of interrupt it is and handle
    struct device *dev = (struct device*)args;
    struct imx_msgunit_state *state = get_state(dev);

    uint32_t flags = MU_GetStatusFlags( get_base(dev) );

    static const int txFlags[MU_TR_COUNT] = {
        kMU_Tx0EmptyFlag, kMU_Tx1EmptyFlag, kMU_Tx2EmptyFlag, kMU_Tx3EmptyFlag
    };

    static const int txFlagsAll = (
        kMU_Tx0EmptyFlag | kMU_Tx1EmptyFlag | kMU_Tx2EmptyFlag | kMU_Tx3EmptyFlag );

    if( flags & txFlagsAll ) {
        for( unsigned i=0; i<MU_TR_COUNT; ++i) {
            if( flags & txFlags[i] )
            {
                invoke_tx_cb(state->tx_callbacks, i);
            }
        }
    }

    static const int rxFlags[MU_RR_COUNT] = {
        kMU_Rx0FullFlag, kMU_Rx1FullFlag, kMU_Rx2FullFlag, kMU_Rx3FullFlag
    };

    static const int rxFlagsAll = (
        kMU_Rx0FullFlag | kMU_Rx1FullFlag | kMU_Rx2FullFlag | kMU_Rx3FullFlag );
    
    if( flags & rxFlagsAll ) {
        for( unsigned i=0; i<MU_RR_COUNT; ++i) {
            if( flags & rxFlags[i] ) {
                uint32_t msg = MU_ReceiveMsg( get_base(dev), i );
                invoke_rx_cb( state->rx_callbacks, i, msg );
            }
        }
    }

    return INT_RESCHEDULE;
}


status_t class_msgunit_register_tx_callback(struct device *dev, uint32_t idx, msgunit_tx_cb_t cb)
{
    struct msgunit_ops *ops = get_ops(dev);
    if( !ops ) {
        return ERR_NOT_CONFIGURED;
    }

    if( ops->register_tx_callback ){
        return ops->register_tx_callback(dev, idx, cb);
    }
    else {
        return ERR_NOT_SUPPORTED;
    }
}

status_t class_msgunit_register_rx_callback(struct device *dev, uint32_t idx, msgunit_rx_cb_t cb)
{
    struct msgunit_ops *ops = get_ops(dev);
    if( !ops ) {
        return ERR_NOT_CONFIGURED;
    }

    if( ops->register_rx_callback ) {
        return ops->register_rx_callback(dev, idx, cb);
    }
    else {
        return ERR_NOT_SUPPORTED;
    }
}

struct device *class_msgunit_get_device_by_id(int id)
{
    TRACE_ENTRY;
    struct device *dev = NULL;
    struct imx_msgunit_state *state = NULL;

    spin_lock_saved_state_t lock_state;
    spin_lock_irqsave(&imx_msgunit_list_lock, lock_state);

    TRACEF("About to try parsing the list...\n");

    list_for_every_entry(&imx_msgunit_list, state, struct imx_msgunit_state, node) {
        if (state->bus_id == id) {
            dev = state->device;
            break;
        }
    }

    spin_unlock_irqrestore(&imx_msgunit_list_lock, lock_state);

    TRACEF("will return %p\n", dev);
    TRACE_EXIT;
    return dev;
}

status_t class_msgunit_send_msg(struct device *dev, uint32_t channel, uint32_t msg)
{
    TRACEF("called with dev: %p, channel: %d, msg: %d\n", dev, channel, msg);
    status_t retval = 0;
    struct msgunit_ops *ops = device_get_driver_ops(dev, struct msgunit_ops, std);
    if( !ops ) {
        TRACEF("Couldn't find driver ops!\n");
        TRACEF("exit at line %d\n", __LINE__);
        return ERR_NOT_CONFIGURED;
    }

    if( ops->send_msg ) {
        retval = ops->send_msg(dev, channel, msg);
        return retval;
    }
    else
    {
        TRACEF("No send_msg function registered!\n");
        return ERR_NOT_SUPPORTED;
    }
}