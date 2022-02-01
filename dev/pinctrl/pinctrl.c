/*!
 * \file pinctrl.c
 * \author D. Anderson
 * \brief 
 * Facilitate access to gpio driver in device-tree systems
 * 
 * Copyright (c) 2022 Presonus Audio Electronics
 * 
 */

#include <dev/class/pinctrl.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <list.h>

#include <kernel/spinlock.h>

#include "trace.h"

#define LOCAL_TRACE 1
#include <dev/class/gpio.h>


#define DRIVER_NAME "pin_controller"

struct pin_controller_state {
    int bus_id;
    struct device *device;
    struct gpio_desc dbg_gpio;
    struct list_node node;
};

static struct list_node imx_pinctrl_list = LIST_INITIAL_VALUE(imx_pinctrl_list);
static spin_lock_t imx_pinctrl_list_lock = SPIN_LOCK_INITIAL_VALUE;

#include <lib/appargs.h>
static status_t pin_controller_init(struct device *dev)
{
    int ret;
    struct pin_controller_state *state = malloc(sizeof(struct pin_controller_state));
    const struct device_config_data *config = dev->config;

    ASSERT(state);
    memset(state, 0, sizeof(struct pin_controller_state));

    state->device = dev;
    state->bus_id = config->bus_id;

    dev->state = state;

    ret = gpio_request_by_name(dev, "dbg-gpio",
                        &state->dbg_gpio,
                        GPIO_DESC_OUTPUT | GPIO_DESC_OUTPUT_ACTIVE);
    if (ret) {
        printf("Failed to request debug gpio for pin_controller: %d\n", ret);
        return ret;
    }

    spin_lock_saved_state_t lock_state;
    spin_lock_irqsave(&imx_pinctrl_list_lock, lock_state);

    list_add_tail(&imx_pinctrl_list, &state->node );

    spin_unlock_irqrestore(&imx_pinctrl_list_lock, lock_state );

    return 0;
}

static status_t pin_controller_open(struct device *dev)
{
    LTRACEF ("pin_controller is open");
    return 0;
}

static status_t pin_controller_close(struct device *dev)
{
    LTRACEF ("pin_controller is closed");
    return 0;
}

static status_t pin_controller_pin_write(struct device *dev, uint8_t pin_idx, uint8_t wr_val)
{
    struct pin_controller_state *state = dev->state;
    gpio_desc_set_value(&state->dbg_gpio, wr_val);
    return 0;
}

static struct pin_controller_ops the_ops = {
        .std = {
            .init = pin_controller_init
        },
        .pin_write = pin_controller_pin_write
};

DRIVER_EXPORT_WITH_LVL(pin_controller, &the_ops.std, DRIVER_INIT_HAL);

struct device *pin_controller_get_device_by_id (int id)
{
    struct device *dev = NULL;
    struct pin_controller_state *state = NULL;

    spin_lock_saved_state_t lock_state;
    spin_lock_irqsave(&imx_pinctrl_list_lock, lock_state);

    list_for_every_entry (&imx_pinctrl_list, state, struct pin_controller_state, node)
    {
        if (state->bus_id == id)
        {
            dev = state->device;
            break;
        }
    }

    spin_unlock_irqrestore (&imx_pinctrl_list_lock, lock_state);
    return dev;
}
