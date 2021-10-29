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
    uint32_t mu_channel;
    uint32_t chan_irq_mask;
    MU_Type *io_base;
    mutex_t mutex;
    struct device *device;
    msgunit_tx_cb_t tx_callback;
    msgunit_rx_cb_t rx_callback;
    struct list_node node;
};

static uint32_t const tx_enable_masks[] = {
    kMU_Tx0EmptyInterruptEnable, kMU_Tx1EmptyInterruptEnable,
    kMU_Tx2EmptyInterruptEnable, kMU_Tx3EmptyInterruptEnable
};

static uint32_t const rx_enable_masks[] = {
    kMU_Rx0FullInterruptEnable, kMU_Rx1FullInterruptEnable,
    kMU_Rx2FullInterruptEnable, kMU_Rx3FullInterruptEnable
};

static uint32_t const gp_enable_masks[] = {
    kMU_GenInt0InterruptEnable, kMU_GenInt1InterruptEnable,
    kMU_GenInt1InterruptEnable, kMU_GenInt3InterruptEnable
};

static uint32_t const per_ch_irq_masks[] = {
    ( kMU_Tx0EmptyFlag | kMU_Rx0FullFlag | kMU_GenInt0Flag ), // Channel 0
    ( kMU_Tx1EmptyFlag | kMU_Rx1FullFlag | kMU_GenInt1Flag ), // Channel 1
    ( kMU_Tx2EmptyFlag | kMU_Rx2FullFlag | kMU_GenInt2Flag ), // Channel 2
    ( kMU_Tx3EmptyFlag | kMU_Rx3FullFlag | kMU_GenInt3Flag ) // Channel 3
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

static inline void mask_all_irq(MU_Type *base, uint32_t channel)
{
    ASSERT( NULL != base );
    uint32_t mask = tx_enable_masks[channel] | rx_enable_masks[channel] | gp_enable_masks[channel];
    base->CR &= ~mask;
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

    status = of_device_get_int32(dev, "channel", &(state->mu_channel) );
    if( NO_ERROR != status )
    {
        TRACEF("Failed to get MU Channel!\n");
        status = ERR_NOT_FOUND;
        goto free_state;
    }

    state->chan_irq_mask = per_ch_irq_masks[state->mu_channel];
    TRACEF("MU Channel is: %d\n", state->mu_channel);


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

    // Make sure our interrupts are masked before proceeding
    mask_all_irq(state->io_base, state->mu_channel);

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

free_state:
    if( state )
    {
        free( state );
    }

    return status;
}

static status_t msgunit_send_msg(struct device *dev, uint32_t msg)
{
    uint32_t reg_idx = get_state(dev)->mu_channel;
    MU_SendMsg( get_base(dev), reg_idx, msg );
    return NO_ERROR;
}

static status_t msgunit_receive_msg(struct device *dev, uint32_t *dst)
{
    ASSERT(dst);
    uint32_t reg_idx = get_state(dev)->mu_channel;
    *dst = MU_ReceiveMsg( get_base(dev), reg_idx);
    return NO_ERROR;
}

static status_t msgunit_register_tx_callback(struct device *dev, msgunit_tx_cb_t cb)
{
    TRACE_ENTRY;

    if( get_state(dev)->tx_callback != NULL ) {
        TRACE_EXIT;
        return ERR_ALREADY_BOUND;
    }

    get_state(dev)->tx_callback = cb;
    TRACE_EXIT;
    return NO_ERROR;
}

static status_t msgunit_register_rx_callback(struct device *dev, msgunit_rx_cb_t cb)
{
    if( get_state(dev)->rx_callback != NULL ) {
        TRACE_EXIT;
        return ERR_ALREADY_BOUND;
    }

    get_state(dev)->rx_callback = cb;
    TRACE_EXIT;
    return NO_ERROR;
}

static status_t msgunit_start(struct device *dev)
{
    TRACE_ENTRY;
    // unmask rx interrupts for the active channel
    get_base(dev)->CR |= ( rx_enable_masks[get_state(dev)->mu_channel] );
    
    // if MU interrupt disabled, enable it
    // if( 0 == GIC_GetEnableIRQ(MU_A53_IRQn) ) {
    //     TRACEF("Enabling MU_A53_IRQn\n");
    //     GIC_EnableIRQ(MU_A53_IRQn);
    // }

    return 0;
}

static status_t msgunit_stop(struct device *dev)
{
    TRACE_ENTRY;
    // mask tx and rx interrupts for the active channel

    ASSERT( dev );
    udelay(2000);
    MU_Type * base = get_base(dev);
    ASSERT ( base );
    udelay(2000);
    struct imx_msgunit_state *state = get_state(dev);
    ASSERT(state);
    udelay(2000);
    base->CR &= ~( tx_enable_masks[state->mu_channel] |
        rx_enable_masks[state->mu_channel] );
    
    // if MU interrupt enabled, disabled it
    // if( GIC_GetEnableIRQ(MU_A53_IRQn) ) {
    //     TRACEF("Disabling MU_A53_IRQn\n");
    //     GIC_DisableIRQ(MU_A53_IRQn);
    // }

    TRACEF("Returning 0\n");
    return 0;
}

static struct device_class msgunit_device_class = {
    .name = "msgunit",
};

static struct msgunit_ops imx_msgunit_ops = {
    .std = {
        .device_class = &msgunit_device_class,
        .init = imx_msgunit_init
    },
    .send_msg = msgunit_send_msg,
    .receive_msg = msgunit_receive_msg,
    .register_tx_callback = msgunit_register_tx_callback,
    .register_rx_callback = msgunit_register_rx_callback,
    .start = msgunit_start,
    .stop = msgunit_stop
};

DRIVER_EXPORT_WITH_LVL(msgunit, &imx_msgunit_ops.std, DRIVER_INIT_PLATFORM);

static enum handler_return msgunit_isr (void *args)
{
    /*
        This driver is intended to be used inside a Jailhouse guest,
        where the Linux host might also be using the MU on a different channel.

        Therefore we need to:
        - not touch any settings not related to our own channel
        - in the ISR, quickly determine if this interrupt pertains to 
        our own channel and if not, get out fast
    */

    struct device *dev = (struct device*)args;
    struct imx_msgunit_state *state = get_state(dev);
    uint32_t flags = MU_GetStatusFlags( state->io_base );
    if( !( flags & state->chan_irq_mask ) )
    {
        return INT_NO_RESCHEDULE;
    }    

    // filter flags we don't care about
    flags = flags & state->chan_irq_mask;
    // TRACEF("flags: 0x%x\n", flags);

    // clear interrupts pertaining to us
    state->io_base->SR |= ( flags );

    // flags = MU_GetStatusFlags( state->io_base ) & state->chan_irq_mask;
    // TRACEF("flags after clear: 0x%x\n", flags);

    static const int txFlags[MU_TR_COUNT] = {
        kMU_Tx0EmptyFlag, kMU_Tx1EmptyFlag, kMU_Tx2EmptyFlag, kMU_Tx3EmptyFlag
    };

    if( flags & txFlags[state->mu_channel] ) {
        if( state->tx_callback ) {
            state->tx_callback();
        }
    }

    static const int rxFlags[MU_RR_COUNT] = {
        kMU_Rx0FullFlag, kMU_Rx1FullFlag, kMU_Rx2FullFlag, kMU_Rx3FullFlag
    };
    
    if( flags & rxFlags[state->mu_channel] ) {
        uint32_t msg = MU_ReceiveMsg( get_base(dev), state->mu_channel );
        if( state->rx_callback ) {
            state->rx_callback(msg);
        }
    }

    return INT_RESCHEDULE;
}


status_t class_msgunit_register_tx_callback(struct device *dev, msgunit_tx_cb_t cb)
{
    TRACE_ENTRY;
    struct msgunit_ops *ops = get_ops(dev);
    if( !ops ) {
        return ERR_NOT_CONFIGURED;
    }

    if( ops->register_tx_callback ){
        return ops->register_tx_callback(dev, cb);
    }
    else {
        return ERR_NOT_SUPPORTED;
    }
}

status_t class_msgunit_register_rx_callback(struct device *dev, msgunit_rx_cb_t cb)
{
    TRACE_ENTRY;
    struct msgunit_ops *ops = get_ops(dev);
    if( !ops ) {
        return ERR_NOT_CONFIGURED;
    }

    if( ops->register_rx_callback ) {
        return ops->register_rx_callback(dev, cb);
    }
    else {
        return ERR_NOT_SUPPORTED;
    }
}

status_t class_msgunit_start(struct device *dev)
{
    TRACE_ENTRY;
    struct msgunit_ops *ops = get_ops(dev);
    if( !ops ) {
        return ERR_NOT_CONFIGURED;
    }

    if( ops->start ) {
        return ops->start(dev);
    }
    else {
        return ERR_NOT_SUPPORTED;
    }
}

status_t class_msgunit_stop(struct device *dev)
{
    TRACE_ENTRY;
    struct msgunit_ops *ops = get_ops(dev);
    if( !ops ) {
        return ERR_NOT_CONFIGURED;
    }

    if( ops->start ) {
        return ops->stop(dev);
    }
    else {
        return ERR_NOT_SUPPORTED;
    }
}

struct device *class_msgunit_get_device_by_id(int id)
{
    TRACEF("id: %d\n", id);
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

status_t class_msgunit_send_msg(struct device *dev, uint32_t msg)
{
    TRACEF("called with dev: %p, msg: 0x%x\n", dev, msg);
    status_t retval = 0;
    struct msgunit_ops *ops = device_get_driver_ops(dev, struct msgunit_ops, std);
    if( !ops ) {
        TRACEF("Couldn't find driver ops!\n");
        TRACEF("exit at line %d\n", __LINE__);
        return ERR_NOT_CONFIGURED;
    }

    if( ops->send_msg ) {
        retval = ops->send_msg(dev, msg);
        return retval;
    }
    else
    {
        TRACEF("No send_msg function registered!\n");
        return ERR_NOT_SUPPORTED;
    }
}