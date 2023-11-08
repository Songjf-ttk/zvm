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
#include <virtualization/vdev/virt_device.h>
#include <virtualization/arm/vgic_v3.h>
#include <virtualization/arm/vgic_common.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define DEV_CFG(dev) \
	((const struct virt_device_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct virt_device_data *)(dev)->data)

/**
 * @brief init virt device for the vm.
*/
static void vm_serial_init(const struct device *dev, struct vm *vm, struct virt_dev *vdev_desc)
{
	bool *bit_addr;
	int ret;
	struct virt_dev *vdev;
	struct virt_irq_desc *desc;
	struct virt_dev_list *dev_lists;

	vdev = (struct virt_dev *)k_malloc(sizeof(struct virt_dev));
	if (!vdev) {
        ZVM_LOG_ERR("Allocate memory for dgconsole error!\n");
        return -EMMAO;
    }

	vdev->dev_pt_flag = true;
	vdev->shareable = false;

	ret = vm_virt_dev_add(vm, vdev, dev->name,
		DEV_CFG(dev)->reg_base, vdev_desc->vm_vdev_paddr,
		DEV_CFG(dev)->reg_size);
	if(ret){
		ZVM_LOG_WARN("Init virt dev error\n");
        return -ENOVDEV;
	}
	/* We should find the default irq info from VM'dts later @TODO */
    vdev->virq = vdev_desc->virq;
    vdev->vm = vm;
	vdev->hirq = DEV_CFG(dev)->hirq_num;

	/* Init virq desc for this irq */
	desc = get_virt_irq_desc(vm->vcpus[DEFAULT_VCPU], vdev->virq);
    desc->virq_flags |= VIRT_IRQ_HW;
    desc->id = VM_VIRT_CONSOLE_IRQ;
    desc->pirq_num = vdev->hirq;
    desc->virq_num = vdev->virq;

	/* set vm's uart irq bit */
	bit_addr = VGIC_DATA_IBP(vm->vm_irq_block_data);
	bit_addr[vdev->hirq] = true;

	vdev_irq_callback_user_data_set(dev, vm_uart_callback, vdev);

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
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
    .virt_irq_callback_set = virt_serial_irq_callback_set,
#endif
#ifdef CONFIG_UART_PORT1
	.device_driver_api = &serial_driver_api;
#endif
};

/**
 * The init function of serial, what will not init
 * the hardware device, but get the device config for
 * zvm system.
*/
static int serial_init(const struct device *dev)
{
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	((const struct uart_device_config * const)(DEV_CFG(dev)->device_config))->irq_config_func(dev);
#endif
	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

void virt_serial_isr(const struct device *dev)
{
	struct virt_device_data *data = DEV_DATA(dev);

	/* Verify if the callback has been registered */
	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb, data->irq_cb_data);
	}
}
#endif

#ifdef CONFIG_VM_SERIAL1

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void serial1_irq_config_func(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(vmserial1)),
		    DT_IRQ(DT_ALIAS(vmserial1), priority),
		    virt_serial_isr,
		    DEVICE_DT_GET(DT_ALIAS(vmserial1)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(vmserial1)));
}
#endif

static struct uart_config serial1_data_port = {
	.baudrate = DT_PROP(DT_ALIAS(vmserial1), current_speed),
};

static struct virt_device_data virt_serial1_data_port = {
	.device_data = &serial1_data_port,
};

static struct uart_device_config serial1_cfg_port = {
	.base = (uint8_t *)DT_REG_ADDR(DT_ALIAS(vmserial1)),
	.sys_clk_freq = DT_PROP_BY_PHANDLE(DT_ALIAS(vmserial1), clocks, clock_frequency),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = serial1_irq_config_func,
#endif
};

static struct virt_device_config virt_serial1_cfg = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmserial1)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmserial1)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmserial1)),
	.device_config = &serial1_cfg_port,
};

/**
 * @brief Define the serial1 description for zvm.
*/
DEVICE_DT_DEFINE(DT_ALIAS(vmserial1),
            &serial_init,
            NULL,
            &virt_serial1_data_port,
            &virt_serial1_cfg, POST_KERNEL,
            CONFIG_SERIAL_INIT_PRIORITY,
            &virt_serial_api);

#endif /* CONFIG_VM_SERIAL1 */

#ifdef CONFIG_VM_SERIAL2

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void serial2_irq_config_func(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(vmserial2)),
		    DT_IRQ(DT_ALIAS(vmserial2), priority),
		    virt_serial_isr,
		    DEVICE_DT_GET(DT_ALIAS(vmserial2)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(vmserial2)));
}
#endif

static struct uart_config serial2_data_port = {
	.baudrate = DT_PROP(DT_ALIAS(vmserial2), current_speed),
};

static struct virt_device_data virt_serial2_data_port = {
	.device_data = &serial2_data_port,
};

static struct uart_device_config serial2_cfg_port = {
	.base = (uint8_t *)DT_REG_ADDR(DT_ALIAS(vmserial2)),
	.sys_clk_freq = DT_PROP_BY_PHANDLE(DT_ALIAS(vmserial2), clocks, clock_frequency),
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.irq_config_func = serial2_irq_config_func,
#endif
};

static struct virt_device_config virt_serial2_cfg = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmserial2)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmserial2)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmserial2)),
	.device_config = &serial2_cfg_port,
};

/**
 * @brief Define the serial2 description for zvm.
*/
DEVICE_DT_DEFINE(DT_ALIAS(vmserial2),
            &serial_init,
            NULL,
            &virt_serial2_data_port,
            &virt_serial2_cfg, POST_KERNEL,
            CONFIG_SERIAL_INIT_PRIORITY,
            &virt_serial_api);

#endif /* CONFIG_VM_SERIAL2 */
