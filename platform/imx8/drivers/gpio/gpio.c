/*
 * The Clear BSD License
 * Copyright 2019-2020 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <dev/class/gpio.h>
#include "platform/gpio.h"
#include "gpio_hw.h"

#include "kernel/thread.h"
#include "kernel/event.h"
#include "kernel/mutex.h"
#include "kernel/spinlock.h"
#include "err.h"
#include <string.h>
#include <lib/appargs.h>
#include <trace.h>

#define LOCAL_TRACE 1

struct gpio_irq_state {
#define MAX_GPIO_IRQ_NAME 8
    char name[MAX_GPIO_IRQ_NAME]; /* GPIO %d */
    GPIO_Type* base;
    unsigned irq;
    unsigned id;
    unsigned isr_bitmap;
    struct gpio_state *state;
    int_handler isr[MAX_GPIO_IRQ_PER_BANK];
    void * args[MAX_GPIO_IRQ_PER_BANK];
    thread_t *thread;
    event_t event;
    spin_lock_t lock;
    mutex_t mutex;
};

#define IMX_GPIO_MAX_PER_BANK 32
#define IMX_GPIO_NAMES_LENGTH 32
struct imx_gpio_host {
    struct device *device;
    GPIO_Type* io_base;
    struct gpio_irq_state irq_state;
    spin_lock_t lock;
    /* What are the banks with a gpio IRQ installed */
    unsigned isr_bitmap;
    char request_name[IMX_GPIO_MAX_PER_BANK][IMX_GPIO_NAMES_LENGTH];
};


static status_t imx_gpio_init(struct device *dev)
{
    printlk(LK_NOTICE, "%s: entry\n", __PRETTY_FUNCTION__);

    const struct device_config_data *config = dev->config;
    ASSERT(config);

    uint32_t ngpios = IMX_GPIO_MAX_PER_BANK;
    of_device_get_int32(dev, "ngpios", &ngpios);
    if (ngpios > IMX_GPIO_MAX_PER_BANK) {
        printlk(LK_ERR, "%s:%d: Invalid ngpios argument (%d) - limit to %d\n",
                        __PRETTY_FUNCTION__, __LINE__, ngpios,  IMX_GPIO_MAX_PER_BANK);
        ngpios = IMX_GPIO_MAX_PER_BANK;
    }

    LTRACEF ("adding gpio controller, name: %s\n", dev->name);
    
    struct gpio_controller *ctrl = gpio_controller_add(
                                dev, ngpios, sizeof(struct imx_gpio_host));
    ASSERT(ctrl);
    printlk(LK_NOTICE, "%s:%d: Gpio %s: %d pins, base %d\n", __PRETTY_FUNCTION__,
            __LINE__, dev->name, ctrl->count, ctrl->base);

    dev->state = ctrl;
    struct imx_gpio_host * gpio_host = gpio_ctrl_to_host(ctrl);

    memset(gpio_host, 0, sizeof(*gpio_host));
    gpio_host->device = dev;

    struct device_cfg_reg *reg =
                        device_config_get_register_by_name(config, "core");
    ASSERT(reg);

    gpio_host->io_base = (GPIO_Type *) reg->vbase;
    ASSERT(gpio_host->io_base);

    spin_lock_init(&gpio_host->lock);

    return 0;
}

static int imx_gpio_request(struct device *dev,
                                    unsigned pin, const char *label)
{
    struct gpio_controller *ctrl = dev->state;
    ASSERT(ctrl);
    struct imx_gpio_host *gpio_host = gpio_ctrl_to_host(ctrl);
    ASSERT(gpio_host);
    const struct device_config_data *config = dev->config;
    ASSERT(config);

    LTRACEF ("dev: %p, pin: %u, label: %s\n", 
        dev, pin, label);

    if (strlen(gpio_host->request_name[pin]) != 0) {
        printlk(LK_ERR, "%s:%d: Pin %d already requested as %s\n",
                __PRETTY_FUNCTION__, __LINE__, pin,
                gpio_host->request_name[pin]);
        return ERR_BUSY;
    }
    /* Pin */
    char pin_name[IMX_GPIO_NAMES_LENGTH];

    /* Try a specialized pinctrl config */
    strncpy(pin_name, label, IMX_GPIO_NAMES_LENGTH);
    printlk(LK_INFO, "%s:%d: Attempting to configure pinctrl label %s\n",
            __PRETTY_FUNCTION__, __LINE__, pin_name);
    int ret = devcfg_set_pins_by_name(config->pins_cfg,
                            config->pins_cfg_cnt, pin_name);
    if (ret != 0) {
        /* Try a specialized pinctrl config */
        sprintf(pin_name, "IO%d", pin);
        printlk(LK_INFO, "%s:%d: Fallback pinctrl label %s configuration\n",
                __PRETTY_FUNCTION__, __LINE__, pin_name);
        ret = devcfg_set_pins_by_name(config->pins_cfg,
                            config->pins_cfg_cnt, pin_name);
        if (ret != 0) {
            printlk(LK_ERR, "%s:%d: %s pinctrl configuration missing, fail!\n",
                    __PRETTY_FUNCTION__, __LINE__, pin_name);
            return ERR_NOT_FOUND;
        }
    }

    strncpy(gpio_host->request_name[pin], label, IMX_GPIO_NAMES_LENGTH);

    return 0;
}

static int imx_gpio_free(struct device *dev, unsigned pin)
{
    struct gpio_controller *ctrl = dev->state;
    ASSERT(ctrl);
    struct imx_gpio_host *gpio_host = gpio_ctrl_to_host(ctrl);
    ASSERT(gpio_host);

    memset(gpio_host->request_name[pin], 0, IMX_GPIO_NAMES_LENGTH);
    /* Todo Release pinctrl configuration */

    return 0;
}

static int imx_gpio_direction_input(struct device *dev, unsigned pin)
{
    struct gpio_controller *ctrl = dev->state;
    ASSERT(ctrl);
    struct imx_gpio_host *gpio_host = gpio_ctrl_to_host(ctrl);
    ASSERT(gpio_host);

    gpio_pin_config_t config = {kGPIO_DigitalInput, 0, kGPIO_NoIntmode};

    spin_lock_saved_state_t lock_state;
    spin_lock_irqsave(&gpio_host->lock, lock_state);

    GPIO_PinInit(gpio_host->io_base, pin, &config);

    spin_unlock_irqrestore(&gpio_host->lock, lock_state);

    return 0;
}

static int imx_gpio_direction_output(struct device *dev, unsigned pin, int value)
{
    struct gpio_controller *ctrl = dev->state;
    ASSERT(ctrl);
    struct imx_gpio_host *gpio_host = gpio_ctrl_to_host(ctrl);
    ASSERT(gpio_host);

    gpio_pin_config_t config = {kGPIO_DigitalOutput, value, kGPIO_NoIntmode};

    spin_lock_saved_state_t lock_state;
    spin_lock_irqsave(&gpio_host->lock, lock_state);

    GPIO_PinInit(gpio_host->io_base, pin, &config);

    spin_unlock_irqrestore(&gpio_host->lock, lock_state);

    return 0;
}

static int imx_gpio_get_value(struct device *dev, unsigned pin)
{
    struct gpio_controller *ctrl = dev->state;
    ASSERT(ctrl);
    struct imx_gpio_host *gpio_host = gpio_ctrl_to_host(ctrl);
    ASSERT(gpio_host);

    return (int) GPIO_PinRead(gpio_host->io_base, pin);
}

static int imx_gpio_set_value(struct device *dev, unsigned pin, int value)
{
    struct gpio_controller *ctrl = dev->state;
    ASSERT(ctrl);
    struct imx_gpio_host *gpio_host = gpio_ctrl_to_host(ctrl);
    ASSERT(gpio_host);

    spin_lock_saved_state_t lock_state;
    spin_lock_irqsave(&gpio_host->lock, lock_state);

    GPIO_PinWrite(gpio_host->io_base, pin, value);

    spin_unlock_irqrestore(&gpio_host->lock, lock_state);

    return 0;
}

static int imx_gpio_get_open_drain(struct device *dev, unsigned pin)
{
    return 0;
}

static int imx_gpio_set_open_drain(struct device *dev, unsigned pin, int value)
{
    return 0;
}

static int imx_gpio_get_io_base (struct device *dev, void **io_base)
{
    if (!dev || !io_base)
        return ERR_INVALID_ARGS;
    
    struct gpio_controller *ctrl = dev->state;
    if (!ctrl)
        return ERR_INVALID_ARGS;
    
    struct imx_gpio_host *gpio_host = gpio_ctrl_to_host(ctrl);
    if (!gpio_host)
        return ERR_INVALID_ARGS;

    void *loc_io_base = gpio_host->io_base;

    if (!loc_io_base)
        return ERR_INVALID_ARGS;

    *io_base = loc_io_base;

    return 0;
}

static struct device_class gpio_device_class = {
    .name = "gpio",
};

struct gpio_ops imx_gpio_ops = {
    .std = {
        .device_class = &gpio_device_class,
        .init = imx_gpio_init,
    },
    .request = imx_gpio_request,
    .free = imx_gpio_free,
    .direction_input = imx_gpio_direction_input,
    .direction_output = imx_gpio_direction_output,
    .get_value = imx_gpio_get_value,
    .set_value = imx_gpio_set_value,
    .get_open_drain = imx_gpio_get_open_drain,
    .set_open_drain = imx_gpio_set_open_drain,
    .get_io_base = imx_gpio_get_io_base
};

DRIVER_EXPORT_WITH_CFG_LVL(gpio, &imx_gpio_ops.std, DRIVER_INIT_CORE, sizeof(struct gpio_irq_state));
