/*
 * Copyright 2023 HNU-ESNL
 * Copyright 2023 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <init.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/uart.h>
#include <virtualization/zvm.h>
#include <virtualization/vm_console.h>
#include <virtualization/vdev/vgic_common.h>
#include <virtualization/vdev/vgic_v3.h>
#include <virtualization/vm_irq.h>
#include <virtualization/vdev/fiq_debugger.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define DEV_CFG(dev) \
	((const struct virt_device_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct virt_device_data *)(dev)->data)

/**
 * @brief init vm serial device for the vm. Including:
 * 1. Allocating virt device to vm, and build map.
 * 2. Setting the device's irq for binding virt interrupt with hardware interrupt.
*/
static int vm_serial_init(const struct device *dev, struct vm *vm, struct virt_dev *vdev_desc)
{
	return 0;
}

/**
 * @brief set the callback function for serial which will be called
 * when virt device is bind to vm.
*/
static void virt_serial_irq_callback_set(const struct device *dev,
						void *cb, void *user_data)
{
	/* Binding to the user's callback function and get the user data. */
	DEV_DATA(dev)->irq_cb = cb;
	DEV_DATA(dev)->irq_cb_data = user_data;
}

static const struct uart_driver_api serial_driver_api;

static const struct virt_device_api virt_serial_api = {
	.init_fn = vm_serial_init,
	.device_driver_api = &serial_driver_api,
};

/**
 * @brief The init function of serial, what will not init
 * the hardware device, but get the device config for
 * zvm system.
*/
static int serial_init(const struct device *dev)
{
	dev->state->init_res = VM_DEVICE_INIT_RES;
	ZVM_LOG_INFO("** Ready to init dev name is: %s. \n", dev->name);
	return 0;
}


static struct virt_device_data virt_serial2_data_port = {
	.device_data = NULL,
};

static struct virt_device_config virt_serial2_cfg = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(linuxdev)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(linuxdev)),
	.hirq_num = DT_IRQN(DT_ALIAS(linuxdev)),
	.device_config = NULL,
};

/**
 * @brief Define the serial2 description for zvm.
*/
DEVICE_DT_DEFINE(DT_ALIAS(linuxdev),
            &serial_init,
            NULL,
            &virt_serial2_data_port,
            &virt_serial2_cfg, POST_KERNEL,
            CONFIG_SERIAL_INIT_PRIORITY,
            &virt_serial_api);

