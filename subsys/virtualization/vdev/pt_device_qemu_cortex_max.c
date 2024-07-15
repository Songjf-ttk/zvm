
#include <kernel.h>
#include <arch/cpu.h>
#include <init.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/uart.h>
#include <virtualization/zvm.h>
#include <virtualization/vm_console.h>
#include <virtualization/vm_device.h>
#include <virtualization/vm_irq.h>
#include <virtualization/vdev/pt_device.h>

//LOG_MODULE_REGISTER(ZVM_MODULE_NAME);

#define DEV_CFG(dev) \
	((const struct virt_device_config * const) \
    (dev)->config)
#define DEV_DATA(dev) \
	((struct virt_device_data *)(dev)->data)

#define PTDEV_CFG(dev) \
	((const struct pass_through_device_config * const) \
	(DEV_CFG(dev)->device_config))

/*Device init function when system bootup. */
static int pass_through_device_init(const struct device *dev)
{
	/*set device type to res.*/
	dev->state->init_res |= VM_DEVICE_INIT_RES;

	/*init the configuration for interrupt.*/
	if(PTDEV_CFG(dev)->irq_config_func){
		PTDEV_CFG(dev)->irq_config_func(dev);
	}

	printk("PT-DEVICE: Initialized pass-through device: %s. \n", dev->name);
	return 0;
}

static int vm_ptdevice_init(const struct device *dev, struct vm *vm, struct virt_dev *vdev_desc)
{
	struct virt_dev *vdev;

    vdev = allocate_device_to_vm(dev, vm, vdev_desc, true, false);
	if(!vdev){
		printk("Init virt serial device error\n");
        return -ENODEV;
	}

	if(DEV_DATA(dev)->device_data){
		struct virt_dev *vdev_tmp = DEV_DATA(dev)->device_data;
		printk("Device data is not NULL, please check the device: %s\n", vdev_tmp->name);
	} else {
		DEV_DATA(dev)->device_data = vdev;
	}

	/*set special function for vm device init*/
	if(PTDEV_CFG(dev)->ptdev_spec_init_func){
		PTDEV_CFG(dev)->ptdev_spec_init_func(vdev);
	}

	return 0;
}

static void pass_through_device_isr(const struct device *dev)
{
	/*irq handler.*/
	if(DEV_DATA(dev)->device_data){
		vm_device_callback_func(dev, NULL, DEV_DATA(dev)->device_data);
	} else{
		printk("irq handle error, vdev is NULL, please check the device: %s\n", dev->name);
	}

    /*set special function for vm device irq route*/
	if(PTDEV_CFG(dev)->ptdev_spec_irq_func){
		PTDEV_CFG(dev)->ptdev_spec_irq_func(dev);
	}
}

static const struct virt_device_api virt_ptdevice_api = {
	.init_fn = vm_ptdevice_init,
};


/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/

/*---------------------Date for each pt device---------------------------*/
/*irq exist? set this configuration.*/
static void ptdevice_irq_config_func_2(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(ptdevice2)),
		    DT_IRQ(DT_ALIAS(ptdevice2), priority),
		    pass_through_device_isr,
		    DEVICE_DT_GET(DT_ALIAS(ptdevice2)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(ptdevice2)));
}
/*irq exist? if not, set function NULL.*/
static struct pass_through_device_config ptdevice_cfg_port_2 = {
	.irq_config_func = ptdevice_irq_config_func_2,
	.ptdev_spec_init_func = NULL,
	.ptdev_spec_irq_func = NULL,
};

static struct virt_device_config virt_ptdevice_cfg_2 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(ptdevice2)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(ptdevice2)),
	.hirq_num = DT_IRQN(DT_ALIAS(ptdevice2)),
	.device_config = &ptdevice_cfg_port_2,
};

static struct virt_device_data virt_ptdevice_data_port_2 = {
	.device_data = NULL,
};

DEVICE_DT_DEFINE(DT_ALIAS(ptdevice2),
            &pass_through_device_init,
            NULL, &virt_ptdevice_data_port_2, &virt_ptdevice_cfg_2,
			POST_KERNEL,
            CONFIG_SERIAL_INIT_PRIORITY,
            &virt_ptdevice_api);
/*------------------------cut line---------------------------------------*/

/*---------------------Date for each pt device---------------------------*/
/*irq exist? set this configuration.*/
static void ptdevice_irq_config_func_1(const struct device *dev)
{
	IRQ_CONNECT(DT_IRQN(DT_ALIAS(ptdevice1)),
		    DT_IRQ(DT_ALIAS(ptdevice1), priority),
		    pass_through_device_isr,
		    DEVICE_DT_GET(DT_ALIAS(ptdevice1)),
		    0);
	irq_enable(DT_IRQN(DT_ALIAS(ptdevice1)));
}
/*irq exist? if not, set function NULL.*/
static struct pass_through_device_config ptdevice_cfg_port_1 = {
	.irq_config_func = ptdevice_irq_config_func_1,
	.ptdev_spec_init_func = NULL,
	.ptdev_spec_irq_func = NULL,
};

static struct virt_device_config virt_ptdevice_cfg_1 = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(ptdevice1)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(ptdevice1)),
	.hirq_num = DT_IRQN(DT_ALIAS(ptdevice1)),
	.device_config = &ptdevice_cfg_port_1,
};

static struct virt_device_data virt_ptdevice_data_port_1 = {
	.device_data = NULL,
};

DEVICE_DT_DEFINE(DT_ALIAS(ptdevice1),
            &pass_through_device_init,
            NULL, &virt_ptdevice_data_port_1, &virt_ptdevice_cfg_1,
			POST_KERNEL, CONFIG_SERIAL_INIT_PRIORITY,
            &virt_ptdevice_api);
/*------------------------cut line---------------------------------------*/