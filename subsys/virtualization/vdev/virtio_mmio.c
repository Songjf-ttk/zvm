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
#include <virtualization/vdev/vgic_v3.h>
#include <virtualization/vdev/vgic_common.h>
#include <virtualization/vdev/virtio/virtio.h>
#include <virtualization/vdev/virtio/virtio_mmio.h>
#include <virtualization/vdev/virtio/virtio_blk.h>

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
static int vm_virtio_mmio_init(const struct device *dev, struct vm *vm, struct virt_dev *vdev_desc)
{
	int ret;

	ret = ((struct virtio_mmio_driver_api *)((struct virt_device_api *)dev->api)->device_driver_api)->probe(vm, vdev_desc);
	if (ret) {
		ZVM_LOG_WARN(" Init virtio device error! \n");
		return -EFAULT;
	}

	ret = ((struct virtio_mmio_driver_api *)((struct virt_device_api *)dev->api)->device_driver_api)->reset(vdev_desc);
	if (ret) {
		ZVM_LOG_WARN(" Reset Init virtio device error! \n");
		return -EFAULT;
	}

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

static int virtio_mmio_config_write(struct virtio_mmio_dev *m,
				    uint32_t offset, void *src, uint32_t src_len)
{
	int rc = 0;
	uint32_t val = *(uint32_t *)(src);

	if (src_len != 4) {
		printk("%s: guest=%s invalid length=%d\n",
			   __func__, m->guest->vm_name, src_len);
		return -EINVAL;
	}

	switch (offset) {
	case VIRTIO_MMIO_HOST_FEATURES_SEL:
		m->config.host_features_sel = val;
		break;
	case VIRTIO_MMIO_GUEST_FEATURES_SEL:
		m->config.guest_features_sel = val;
		break;
	case VIRTIO_MMIO_GUEST_FEATURES:
		m->dev.emu->set_guest_features(&m->dev,
					m->config.guest_features_sel, val);
		break;
	case VIRTIO_MMIO_GUEST_PAGE_SIZE:
		m->config.guest_page_size = val;
		break;
	case VIRTIO_MMIO_QUEUE_SEL:
		m->config.queue_sel = val;
		break;
	case VIRTIO_MMIO_QUEUE_NUM:
		m->config.queue_num = val;
		m->dev.emu->set_size_vq(&m->dev,
					m->config.queue_sel,
					m->config.queue_num);
		break;
	case VIRTIO_MMIO_QUEUE_ALIGN:
		m->config.queue_align = val;
		break;
	case VIRTIO_MMIO_QUEUE_PFN:
		m->dev.emu->init_vq(&m->dev,
				    m->config.queue_sel,
				    m->config.guest_page_size,
				    m->config.queue_align,
				    val);
		break;
	case VIRTIO_MMIO_QUEUE_NOTIFY:
		m->dev.emu->notify_vq(&m->dev, val);
		break;
	case VIRTIO_MMIO_INTERRUPT_ACK:
		m->config.interrupt_status &= ~val;
		rc = set_virq_to_vm(m->guest, m->irq);
		if(rc < 0){
			printk("Send virq to vm error!\n");
		}
		break;
	case VIRTIO_MMIO_STATUS:
		if (val != m->config.status) {
			m->dev.emu->status_changed(&m->dev, val);
		}
		m->config.status = val;
		break;
	default:
		printk("%s: guest=%s invalid offset=0x%x\n",
			   __func__, m->guest->vm_name, offset);
		rc = -EINVAL;
		break;
	};

	return rc;
}

int virtio_mmio_write(struct virt_dev *edev,
			     uint32_t offset, uint32_t src_mask, uint32_t src, uint32_t src_len)
{
	struct virtio_mmio_dev *m = edev->priv_vdev;
	src = src & ~src_mask;

	/* Device specific config write */
	if (offset >= VIRTIO_MMIO_CONFIG) {
		offset -= VIRTIO_MMIO_CONFIG;
		return virtio_config_write(&m->dev, offset, &src, src_len);
	}

	return virtio_mmio_config_write(m, offset, &src, src_len);
}

static int virtio_mmio_config_read(struct virtio_mmio_dev *m,
			    uint32_t offset, void *dst, uint32_t dst_len)
{
	int rc = 0;

	if (dst_len != 4) {
		printk("%s: guest=%s invalid length=%d\n",
			   __func__, m->guest->vm_name, dst_len);
		return -EINVAL;
	}

	switch (offset) {
	case VIRTIO_MMIO_MAGIC_VALUE:
		*(uint32_t *)dst = *((uint32_t *)((void *)&m->config.magic[0]));
		break;
	case VIRTIO_MMIO_VERSION:
		*(uint32_t *)dst = *((uint32_t *)((void *)&m->config.version));
		break;
	case VIRTIO_MMIO_DEVICE_ID:
		*(uint32_t *)dst = *((uint32_t *)((void *)&m->config.device_id));
		break;
	case VIRTIO_MMIO_VENDOR_ID:
		*(uint32_t *)dst = *((uint32_t *)((void *)&m->config.vendor_id));
		break;
	case VIRTIO_MMIO_INTERRUPT_STATUS:
		*(uint32_t *)dst = *((uint32_t *)((void *)&m->config.interrupt_status));
		break;
	case VIRTIO_MMIO_HOST_FEATURES:
		if (m->config.host_features_sel == 0)
			*(uint32_t *)dst =
			(uint32_t)m->dev.emu->get_host_features(&m->dev);
		else
			*(uint32_t *)dst =
			(uint32_t)(m->dev.emu->get_host_features(&m->dev) >> 32);
		break;
	case VIRTIO_MMIO_QUEUE_PFN:
		*(uint32_t *)dst = m->dev.emu->get_pfn_vq(&m->dev,
					     m->config.queue_sel);
		break;
	case VIRTIO_MMIO_QUEUE_NUM_MAX:
		*(uint32_t *)dst = m->dev.emu->get_size_vq(&m->dev,
					      m->config.queue_sel);
		break;
	case VIRTIO_MMIO_STATUS:
		*(uint32_t *)dst = *((uint32_t *)((void *)&m->config.status));
		break;
	default:
		printk("%s: guest=%s invalid offset=0x%x\n",
			   __func__, m->guest->vm_name, offset);
		rc = -EINVAL;
		break;
	}

	return rc;
}

int virtio_mmio_read(struct virt_dev *edev,
			    uint32_t offset, uint32_t *dst, uint32_t dst_len)
{
	struct virtio_mmio_dev *m = edev->priv_vdev;
	/* Device specific config write */
	if (offset >= VIRTIO_MMIO_CONFIG) {
		offset -= VIRTIO_MMIO_CONFIG;
		return virtio_config_read(&m->dev, offset, dst, dst_len);
	}

	return virtio_mmio_config_read(m, offset, dst, dst_len);
}

static int virtio_mmio_notify(struct virtio_device *dev, uint32_t vq)
{	
	int err = 0;
	struct virtio_mmio_dev *m = dev->tra_data;

	m->config.interrupt_status |= VIRTIO_MMIO_INT_VRING;
	
	err = set_virq_to_vm(dev->guest, m->irq);
	if(err < 0){
		printk("Send virq to vm error!\n");
		return -EFAULT;
	}

	return 0;
}

static struct virtio_transport mmio_tra = {
	.name = "virtio_mmio",
	.notify = virtio_mmio_notify,
};

int virtio_mmio_probe(struct vm *guest, struct virt_dev *edev) {
	int rc = 0;
	uint32_t name_len;
	struct virtio_mmio_dev *m;

	m = (struct virtio_mmio_dev *)k_malloc(sizeof(struct virtio_mmio_dev));
	if (!m) {
		rc = -ENOMEM;
		goto virtio_mmio_probe_done;
	}

	m->guest = guest;

	name_len = strlen(guest->vm_name) < VIRTIO_DEVICE_MAX_NAME_LEN ? strlen(guest->vm_name) : VIRTIO_DEVICE_MAX_NAME_LEN;
	strncpy(m->dev.name, guest->vm_name, name_len);
	name_len = name_len + strlen(edev->name) < VIRTIO_DEVICE_MAX_NAME_LEN ? strlen(edev->name) : VIRTIO_DEVICE_MAX_NAME_LEN - name_len;
	strncat(m->dev.name, edev->name, name_len);
	m->dev.name[strlen(m->dev.name)] = '\0';

	m->dev.edev = edev;
	m->dev.id.type = ((struct virtio_device_config *)((struct virt_device_config *)((struct device *)edev->priv_data)->config)->device_config)->virtio_type;
	m->dev.tra = &mmio_tra;
	m->dev.tra_data = m;
	m->dev.guest = guest;

	m->config = (struct virtio_mmio_config) {
		     .magic          = {'v', 'i', 'r', 't'},
		     .version        = 1,
		     .vendor_id      = 0x52535658, /* XVSR */
		     .queue_num_max  = 256,
	};

	m->config.device_id = m->dev.id.type;
	m->irq = edev->virq;

	if ((rc = virtio_register_device(&m->dev))) {
		goto virtio_mmio_probe_freestate_fail;
	}

	edev->priv_vdev = m;

	goto virtio_mmio_probe_done;

virtio_mmio_probe_freestate_fail:
	k_free(m);
virtio_mmio_probe_done:
	return rc;
}

static int virtio_mmio_remove(struct virt_dev *edev)
{
	struct virtio_mmio_dev *m = edev->priv_vdev;

	if (m) {
		virtio_unregister_device(&m->dev);
		k_free(m);
		edev->priv_vdev = NULL;
	}

	return 0;
}

static int virtio_mmio_reset(struct virt_dev *edev)
{
	struct virtio_mmio_dev *m = edev->priv_vdev;

	m->config.host_features_sel = 0x0;
	m->config.guest_features_sel = 0x0;
	m->config.queue_sel = 0x0;
	m->config.interrupt_status = 0x0;
	m->config.status = 0x0;

	return virtio_reset(&m->dev);
}

static const struct virtio_mmio_driver_api virt_mmio_driver_api = {
	.write = virtio_mmio_write,
	.read = virtio_mmio_read,
	.probe = virtio_mmio_probe,
	.remove = virtio_mmio_remove,
	.reset = virtio_mmio_reset,
};

static const struct virt_device_api virt_virtio_mmio_api = {
	.init_fn = vm_virtio_mmio_init,
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

static struct virtio_mmio_dev virtio_mmio_dev1;
static struct virt_device_data virt_mmio_data_port_1 = {
	.device_data = &virtio_mmio_dev1,
};

static struct virtio_mmio_dev virtio_mmio_dev2;
static struct virt_device_data virt_mmio_data_port_2 = {
	.device_data = &virtio_mmio_dev2,
};

static struct virtio_mmio_dev virtio_mmio_dev3;
static struct virt_device_data virt_mmio_data_port_3 = {
	.device_data = &virtio_mmio_dev3,
};

static struct virtio_mmio_dev virtio_mmio_dev4;
static struct virt_device_data virt_mmio_data_port_4 = {
	.device_data = &virtio_mmio_dev4,
};

static struct virtio_mmio_dev virtio_mmio_dev5;
static struct virt_device_data virt_mmio_data_port_5 = {
	.device_data = &virtio_mmio_dev5,
};

static struct virtio_mmio_dev virtio_mmio_dev6;
static struct virt_device_data virt_mmio_data_port_6 = {
	.device_data = &virtio_mmio_dev6,
};

static struct virtio_mmio_dev virtio_mmio_dev7;
static struct virt_device_data virt_mmio_data_port_7 = {
	.device_data = &virtio_mmio_dev7,
};

static struct virtio_mmio_dev virtio_mmio_dev8;
static struct virt_device_data virt_mmio_data_port_8 = {
	.device_data = &virtio_mmio_dev8,
};

static struct virtio_mmio_dev virtio_mmio_dev9;
static struct virt_device_data virt_mmio_data_port_9 = {
	.device_data = &virtio_mmio_dev9,
};

static struct virtio_mmio_dev virtio_mmio_dev10;
static struct virt_device_data virt_mmio_data_port_10 = {
	.device_data = &virtio_mmio_dev10,
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
	.virtio_type = DT_PROP(DT_ALIAS(vmvirtio1), virtio_type),
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
	.virtio_type = DT_PROP(DT_ALIAS(vmvirtio2), virtio_type),
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
	.virtio_type = DT_PROP(DT_ALIAS(vmvirtio3), virtio_type),	
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
	.virtio_type = DT_PROP(DT_ALIAS(vmvirtio4), virtio_type),	
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
	.virtio_type = DT_PROP(DT_ALIAS(vmvirtio5), virtio_type),	
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
	.virtio_type = DT_PROP(DT_ALIAS(vmvirtio6), virtio_type),	
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
	.virtio_type = DT_PROP(DT_ALIAS(vmvirtio7), virtio_type),	
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
	.virtio_type = DT_PROP(DT_ALIAS(vmvirtio8), virtio_type),	
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
	.virtio_type = DT_PROP(DT_ALIAS(vmvirtio9), virtio_type),	
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
	.virtio_type = DT_PROP(DT_ALIAS(vmvirtio10), virtio_type),	
#ifdef CONFIG_VIRTIO_INTERRUPT_DRIVEN
	.irq_config_func = virtio_mmio_irq_config_func_10,
#endif
};

static struct virt_device_config virt_virt_mmio_cfg_1 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio1)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio1)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio1)),
	.device_type = DT_PROP(DT_ALIAS(vmvirtio1), device_type),
    .device_config = &virtio_mmio_cfg_port_1,
};

static struct virt_device_config virt_virt_mmio_cfg_2 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio2)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio2)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio2)),
	.device_type = DT_PROP(DT_ALIAS(vmvirtio2), device_type),
    .device_config = &virtio_mmio_cfg_port_2,
};

static struct virt_device_config virt_virt_mmio_cfg_3 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio3)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio3)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio3)),
	.device_type = DT_PROP(DT_ALIAS(vmvirtio3), device_type),
    .device_config = &virtio_mmio_cfg_port_3,
};

static struct virt_device_config virt_virt_mmio_cfg_4 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio4)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio4)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio4)),
	.device_type = DT_PROP(DT_ALIAS(vmvirtio4), device_type),
    .device_config = &virtio_mmio_cfg_port_4,
};

static struct virt_device_config virt_virt_mmio_cfg_5 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio5)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio5)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio5)),
	.device_type = DT_PROP(DT_ALIAS(vmvirtio5), device_type),
    .device_config = &virtio_mmio_cfg_port_5,
};

static struct virt_device_config virt_virt_mmio_cfg_6 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio6)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio6)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio6)),
	.device_type = DT_PROP(DT_ALIAS(vmvirtio6), device_type),
    .device_config = &virtio_mmio_cfg_port_6,
};

static struct virt_device_config virt_virt_mmio_cfg_7 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio7)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio7)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio7)),
	.device_type = DT_PROP(DT_ALIAS(vmvirtio7), device_type),
    .device_config = &virtio_mmio_cfg_port_7,
};

static struct virt_device_config virt_virt_mmio_cfg_8 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio8)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio8)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio8)),
	.device_type = DT_PROP(DT_ALIAS(vmvirtio8), device_type),
    .device_config = &virtio_mmio_cfg_port_8,
};

static struct virt_device_config virt_virt_mmio_cfg_9 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio9)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio9)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio9)),
	.device_type = DT_PROP(DT_ALIAS(vmvirtio9), device_type),
    .device_config = &virtio_mmio_cfg_port_9,
};

static struct virt_device_config virt_virt_mmio_cfg_10 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtio10)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtio10)),
	.hirq_num = DT_IRQN(DT_ALIAS(vmvirtio10)),
	.device_type = DT_PROP(DT_ALIAS(vmvirtio10), device_type),
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
