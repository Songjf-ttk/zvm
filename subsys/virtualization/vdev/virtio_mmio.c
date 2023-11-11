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
#include <virtualization/zvm.h>
#include <virtualization/vdev/virt_device.h>
#include <virtualization/vdev/virt_mmio.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define DEV_CFG(dev) \
	((const struct virt_device_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct virt_device_data *)(dev)->data)

/**
 * @brief init vm virtio mmio device for the vm. Including:
 * 1. Allocating virt device to vm, and build map.
 * 2. Setting the device's irq for binding virt interrupt with hardware interrupt.
*/
static void vm_virio_mmio_init(const struct device *dev, struct vm *vm, struct virt_dev *vdev_desc)
{
	bool *bit_addr;
	int ret;
	struct virt_dev *vdev;

    vdev = allocate_device_to_vm(dev, vm, vdev_desc, true, false);
	if(!vdev){
		ZVM_LOG_WARN("Init virt serial device error\n");
        return;
	}

	vdev_irq_callback_user_data_set(dev, vm_device_callback_func, vdev);

	return 0;
}

/**
 * @brief set the callback function for virtio_mmio which will be called
 * when virt device is bind to vm.
*/
static void virt_virtio_mmio_irq_callback_set(const struct device *dev,
						void *cb, void *user_data)
{
	/* Binding to the user's callback function and get the user data. */
	DEV_DATA(dev)->irq_cb = cb;
	DEV_DATA(dev)->irq_cb_data = user_data;
}

static const struct virtio_mmio_driver_api virt_mmio_driver_api;

static const struct virt_device_api virt_virtio_mmio_api = {
	.init_fn = vm_virio_mmio_init,
#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
    .virt_irq_callback_set = virt_virtio_mmio_irq_callback_set,
#endif
	.device_driver_api = &virt_mmio_driver_api,
};

/**
 * @brief The init function of virt_mmio, what will not init
 * the hardware device, but get the device config for
 * zvm system.
*/
static int virtio_mmio_init(const struct device *dev)
{
	dev->state->init_res = VM_DEVICE_INIT_RES;
#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
	((const struct virtio_device_config * const)(DEV_CFG(dev)->device_config))->irq_config_func(dev);
#endif
	return 0;
}

#ifdef CONFIG_VM_VIRTIO_MMIO

static struct virt_device_data virt_mmio_data_port_1 = {
	.device_data = NULL,
};

static struct virt_device_data virt_mmio_data_port_2 = {
	.device_data = NULL,
};

static struct virt_device_data virt_mmio_data_port_3 = {
	.device_data = NULL,
};

static struct virt_device_data virt_mmio_data_port_4 = {
	.device_data = NULL,
};

static struct virt_device_data virt_mmio_data_port_5 = {
	.device_data = NULL,
};

static struct virt_device_data virt_mmio_data_port_6 = {
	.device_data = NULL,
};

static struct virt_device_data virt_mmio_data_port_7 = {
	.device_data = NULL,
};

static struct virt_device_data virt_mmio_data_port_8 = {
	.device_data = NULL,
};

static struct virt_device_data virt_mmio_data_port_9 = {
	.device_data = NULL,
};

static struct virt_device_data virt_mmio_data_port_10 = {
	.device_data = NULL,
};

#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN

void virt_virtio_mmio_isr(const struct device *dev)
{
	struct virt_device_data *data = DEV_DATA(dev);

	/* Verify if the callback has been registered */
	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb, data->irq_cb_data);
	}
}
#endif

#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
static void virtio_mmio_irq_config_func_1(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(vmvirtio1)),
		    DT_IRQ(DT_ALIAS(vmvirtio1), priority),
		    virt_virtio_mmio_isr,
		    DEVICE_DT_GET(DT_ALIAS(vmvirtio1)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(vmvirtio1)));
}
#endif

static struct virtio_device_config virtio_mmio_cfg_port_1 = {
#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
	.irq_config_func = virtio_mmio_irq_config_func_1,
#endif
};

#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
static void virtio_mmio_irq_config_func_2(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(vmvirtio2)),
		    DT_IRQ(DT_ALIAS(vmvirtio2), priority),
		    virt_virtio_mmio_isr,
		    DEVICE_DT_GET(DT_ALIAS(vmvirtio2)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(vmvirtio2)));
}
#endif

static struct virtio_device_config virtio_mmio_cfg_port_2 = {
#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
	.irq_config_func = virtio_mmio_irq_config_func_2,
#endif
};

#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
static void virtio_mmio_irq_config_func_3(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(vmvirtio3)),
		    DT_IRQ(DT_ALIAS(vmvirtio3), priority),
		    virt_virtio_mmio_isr,
		    DEVICE_DT_GET(DT_ALIAS(vmvirtio3)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(vmvirtio3)));
}
#endif

static struct virtio_device_config virtio_mmio_cfg_port_3 = {
#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
	.irq_config_func = virtio_mmio_irq_config_func_3,
#endif
};

#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
static void virtio_mmio_irq_config_func_4(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(vmvirtio4)),
		    DT_IRQ(DT_ALIAS(vmvirtio4), priority),
		    virt_virtio_mmio_isr,
		    DEVICE_DT_GET(DT_ALIAS(vmvirtio4)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(vmvirtio4)));
}
#endif

static struct virtio_device_config virtio_mmio_cfg_port_4 = {
#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
	.irq_config_func = virtio_mmio_irq_config_func_4,
#endif
};

#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
static void virtio_mmio_irq_config_func_5(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(vmvirtio5)),
		    DT_IRQ(DT_ALIAS(vmvirtio5), priority),
		    virt_virtio_mmio_isr,
		    DEVICE_DT_GET(DT_ALIAS(vmvirtio5)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(vmvirtio5)));
}
#endif

static struct virtio_device_config virtio_mmio_cfg_port_5 = {
#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
	.irq_config_func = virtio_mmio_irq_config_func_5,
#endif
};

#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
static void virtio_mmio_irq_config_func_6(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(vmvirtio6)),
		    DT_IRQ(DT_ALIAS(vmvirtio6), priority),
		    virt_virtio_mmio_isr,
		    DEVICE_DT_GET(DT_ALIAS(vmvirtio6)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(vmvirtio6)));
}
#endif

static struct virtio_device_config virtio_mmio_cfg_port_6 = {
#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
	.irq_config_func = virtio_mmio_irq_config_func_6,
#endif
};

#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
static void virtio_mmio_irq_config_func_7(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(vmvirtio7)),
		    DT_IRQ(DT_ALIAS(vmvirtio7), priority),
		    virt_virtio_mmio_isr,
		    DEVICE_DT_GET(DT_ALIAS(vmvirtio7)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(vmvirtio7)));
}
#endif

static struct virtio_device_config virtio_mmio_cfg_port_7 = {
#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
	.irq_config_func = virtio_mmio_irq_config_func_7,
#endif
};

#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
static void virtio_mmio_irq_config_func_8(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(vmvirtio8)),
		    DT_IRQ(DT_ALIAS(vmvirtio8), priority),
		    virt_virtio_mmio_isr,
		    DEVICE_DT_GET(DT_ALIAS(vmvirtio8)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(vmvirtio8)));
}
#endif

static struct virtio_device_config virtio_mmio_cfg_port_8 = {
#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
	.irq_config_func = virtio_mmio_irq_config_func_8,
#endif
};

#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
static void virtio_mmio_irq_config_func_9(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(vmvirtio9)),
		    DT_IRQ(DT_ALIAS(vmvirtio9), priority),
		    virt_virtio_mmio_isr,
		    DEVICE_DT_GET(DT_ALIAS(vmvirtio9)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(vmvirtio9)));
}
#endif

static struct virtio_device_config virtio_mmio_cfg_port_9 = {
#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
	.irq_config_func = virtio_mmio_irq_config_func_9,
#endif
};

#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
static void virtio_mmio_irq_config_func_10(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(vmvirtio10)),
		    DT_IRQ(DT_ALIAS(vmvirtio10), priority),
		    virt_virtio_mmio_isr,
		    DEVICE_DT_GET(DT_ALIAS(vmvirtio10)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(vmvirtio10)));
}
#endif

static struct virtio_device_config virtio_mmio_cfg_port_10 = {
#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
	.irq_config_func = virtio_mmio_irq_config_func_10,
#endif
};

static struct virt_device_config virt_virt_mmio_cfg_1 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio1)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio1)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio1)),
    .device_config = &virtio_mmio_cfg_port_1,
};

static struct virt_device_config virt_virt_mmio_cfg_2 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio2)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio2)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio2)),
    .device_config = &virtio_mmio_cfg_port_2,
};

static struct virt_device_config virt_virt_mmio_cfg_3 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio3)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio3)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio3)),
    .device_config = &virtio_mmio_cfg_port_3,
};

static struct virt_device_config virt_virt_mmio_cfg_4 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio4)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio4)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio4)),
    .device_config = &virtio_mmio_cfg_port_4,
};

static struct virt_device_config virt_virt_mmio_cfg_5 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio5)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio5)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio5)),
    .device_config = &virtio_mmio_cfg_port_5,
};

static struct virt_device_config virt_virt_mmio_cfg_6 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio6)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio6)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio6)),
    .device_config = &virtio_mmio_cfg_port_6,
};

static struct virt_device_config virt_virt_mmio_cfg_7 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio7)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio7)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio7)),
    .device_config = &virtio_mmio_cfg_port_7,
};

static struct virt_device_config virt_virt_mmio_cfg_8 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio8)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio8)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio8)),
    .device_config = &virtio_mmio_cfg_port_8,
};

static struct virt_device_config virt_virt_mmio_cfg_9 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio9)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio9)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio9)),
    .device_config = &virtio_mmio_cfg_port_9,
};

static struct virt_device_config virt_virt_mmio_cfg_10 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio10)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio10)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio10)),
    .device_config = &virtio_mmio_cfg_port_10,
};

DEVICE_DT_DEFINE(DT_ALIAS(vmvirtio1),
            &virtio_mmio_init,
            NULL,
            &virt_mmio_data_port_1,
            &virt_virt_mmio_cfg_1, POST_KERNEL,
            CONFIG_VIRTIO_MMIO_INIT_PRIORITY,
            &virt_virtio_mmio_api);

DEVICE_DT_DEFINE(DT_ALIAS(vmvirtio2),
            &virtio_mmio_init,
            NULL,
            &virt_mmio_data_port_2,
            &virt_virt_mmio_cfg_2, POST_KERNEL,
            CONFIG_VIRTIO_MMIO_INIT_PRIORITY,
            &virt_virtio_mmio_api);

DEVICE_DT_DEFINE(DT_ALIAS(vmvirtio3),
            &virtio_mmio_init,
            NULL,
            &virt_mmio_data_port_3,
            &virt_virt_mmio_cfg_3, POST_KERNEL,
            CONFIG_VIRTIO_MMIO_INIT_PRIORITY,
            &virt_virtio_mmio_api);

DEVICE_DT_DEFINE(DT_ALIAS(vmvirtio4),
            &virtio_mmio_init,
            NULL,
            &virt_mmio_data_port_4,
            &virt_virt_mmio_cfg_4, POST_KERNEL,
            CONFIG_VIRTIO_MMIO_INIT_PRIORITY,
            &virt_virtio_mmio_api);

DEVICE_DT_DEFINE(DT_ALIAS(vmvirtio5),
            &virtio_mmio_init,
            NULL,
            &virt_mmio_data_port_5,
            &virt_virt_mmio_cfg_5, POST_KERNEL,
            CONFIG_VIRTIO_MMIO_INIT_PRIORITY,
            &virt_virtio_mmio_api);

DEVICE_DT_DEFINE(DT_ALIAS(vmvirtio6),
            &virtio_mmio_init,
            NULL,
            &virt_mmio_data_port_6,
            &virt_virt_mmio_cfg_6, POST_KERNEL,
            CONFIG_VIRTIO_MMIO_INIT_PRIORITY,
            &virt_virtio_mmio_api);

DEVICE_DT_DEFINE(DT_ALIAS(vmvirtio7),
            &virtio_mmio_init,
            NULL,
            &virt_mmio_data_port_7,
            &virt_virt_mmio_cfg_7, POST_KERNEL,
            CONFIG_VIRTIO_MMIO_INIT_PRIORITY,
            &virt_virtio_mmio_api);

DEVICE_DT_DEFINE(DT_ALIAS(vmvirtio8),
            &virtio_mmio_init,
            NULL,
            &virt_mmio_data_port_8,
            &virt_virt_mmio_cfg_8, POST_KERNEL,
            CONFIG_VIRTIO_MMIO_INIT_PRIORITY,
            &virt_virtio_mmio_api);

DEVICE_DT_DEFINE(DT_ALIAS(vmvirtio9),
            &virtio_mmio_init,
            NULL,
            &virt_mmio_data_port_9,
            &virt_virt_mmio_cfg_9, POST_KERNEL,
            CONFIG_VIRTIO_MMIO_INIT_PRIORITY,
            &virt_virtio_mmio_api);

DEVICE_DT_DEFINE(DT_ALIAS(vmvirtio10),
            &virtio_mmio_init,
            NULL,
            &virt_mmio_data_port_10,
            &virt_virt_mmio_cfg_10, POST_KERNEL,
            CONFIG_VIRTIO_MMIO_INIT_PRIORITY,
            &virt_virtio_mmio_api);

#endif /* CONFIG_VM_VIRTIO_MMIO */
