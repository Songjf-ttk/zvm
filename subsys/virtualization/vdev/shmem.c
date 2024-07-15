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
#include <virtualization/arm/mm.h>
#include <virtualization/vm_console.h>
#include <virtualization/vdev/vgic_v3.h>
#include <virtualization/vdev/vgic_common.h>
#include <virtualization/vm_irq.h>
#include <virtualization/vm_mm.h>
#include <virtualization/vdev/shmem.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define DEV_CFG(dev) ((const struct virt_device_config *const)(dev)->config)
#define DEV_DATA(dev) ((struct virt_device_data *)(dev)->data)

#define ZEPHYR_SHMEM_ADDR 0x20000000
#define LINUX_SHMEM_ADDR 0x80000000

static struct virtual_device_instance *mem_instance;

/**
 * @brief Initialize virtual device's memory
*/
static int vm_virt_mem_init(const struct device *dev, struct vm *vm, struct virt_dev *vdev_desc)
{
	struct virt_dev *vdev;
	struct mem_vdevice *mem;
	struct zvm_dev_lists* vdev_list;
	struct  _dnode *d_node, *ds_node;
    struct virt_dev *vm_dev;
	char *dev_name = "VM_SHMEM";
	mem = (struct mem_vdevice *)k_malloc(sizeof(struct mem_vdevice));
    if (!mem) {
        ZVM_LOG_ERR("Allocate memory for mem error \n");
        return -ENODEV;
    }
	mem->mem_base = DT_REG_ADDR(DT_ALIAS(vmvirtmem));
	mem->mem_size = DT_REG_SIZE(DT_ALIAS(vmvirtmem));

	mem_instance = (struct virtual_device_instance *)k_malloc(sizeof(struct virtual_device_instance));
	if(mem_instance == NULL){
		ZVM_LOG_ERR("Allocate memory for mem_instance error \n");
        return -ENODEV;
	}
	mem_instance->data = DEV_DATA(dev);
	mem_instance->cfg = DEV_CFG(dev);
	mem_instance->api = dev->api;
	mem_instance->name = dev->name;
	DEV_DATA(mem_instance)->vdevice_type |= VM_DEVICE_PRE_KERNEL_1;

	bool chosen_flag=false;
	vdev_list = get_zvm_dev_lists();
    SYS_DLIST_FOR_EACH_NODE_SAFE(&vdev_list->dev_idle_list, d_node, ds_node){
        vm_dev = CONTAINER_OF(d_node, struct virt_dev, vdev_node);
        /* host uart device ? */
        if(!strcmp(dev_name, vm_dev->name)){
            chosen_flag = true;
            break;
        }
    }

	if(chosen_flag)
	{
		vdev = vm_virt_dev_add(vm, dev->name, false, false,
								DT_REG_ADDR(DT_ALIAS(vmvirtmem)),
								DT_REG_ADDR(DT_ALIAS(vmvirtmem)),
								vm_dev->vm_vdev_size,
								VM_DEVICE_INVALID_VIRQ, VM_DEVICE_INVALID_VIRQ);
		if(!vdev){
			ZVM_LOG_WARN("Init virt virt_mem device error\n");
        	return -ENODEV;
		}
		vdev->priv_data = mem_instance;
		vdev->priv_vdev = dev;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                       ;
		if(vm->os->type == 1){
			sys_dlist_remove(&vdev->vdev_node);
			sys_dlist_append(&vdev_list->dev_used_list, &vdev->vdev_node);
		}

		ZVM_LOG_INFO("** Add %s device to vm successful. \n", dev->name);

		return 0;
	}
	return -ENODEV;
}

/**
 * @brief Memory read operation
*/
int memory_read(struct virt_dev *vdev, uint64_t addr, uint64_t *value)
{
	uint64_t read_value;

	read_value = sys_read64(vdev->vm_vdev_paddr);
	ZVM_LOG_INFO("vm_vdev_paddr: 0x%x, read_value: 0x%llx\n", vdev->vm_vdev_paddr, read_value);
	*(uint64_t *)value = read_value;
	return read_value;
}

/**
 * @brief Memory write operation
*/
int memory_write(struct virt_dev *vdev, uint64_t addr, uint64_t *value)
{
	uint64_t value_w = *(uint64_t *)value;

	ZVM_LOG_INFO("vm_vdev_paddr: 0x%x, value: 0x%llx\n", vdev->vm_vdev_paddr, value_w);
	sys_write64(value_w, vdev->vm_vdev_paddr);

	return 0;
}

static const struct virt_device_api virt_mem_api = {
	.init_fn = vm_virt_mem_init,
	.virt_device_write = memory_write,
	.virt_device_read = memory_read,
};

static int mem_init(const struct device *dev)
{
	dev->state->init_res = VM_DEVICE_INIT_RES;
	return 0;
}

int vm_shmem_create(struct vm *vm)
{
	int ret = 0;
	const struct device *dev = DEVICE_DT_GET(DT_ALIAS(vmvirtmem));

	if(((const struct virt_device_api * const)(dev->api))->init_fn){
		((const struct virt_device_api * const)(dev->api))->init_fn(dev, vm, NULL);
	}else{
		ZVM_LOG_ERR("No mem device api! \n");
		return -ENODEV;
	}
	return ret;
}

#ifdef CONFIG_VM_SHMEM
static struct shared_mem_config mem_data_port = {
	.base = DT_REG_ADDR(DT_ALIAS(vmvirtmem)),
	.size = DT_REG_SIZE(DT_ALIAS(vmvirtmem)),
};

static struct virt_device_data virt_mem_data_port = {
	.device_data = &mem_data_port,
};

static struct virt_device_config virt_mem_cfg = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmvirtmem)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmvirtmem)),
	.device_config = &virt_mem_data_port,
};

/**
 * @brief Define the mem description for zvm.
*/
DEVICE_DT_DEFINE(DT_ALIAS(vmvirtmem),
				&mem_init,
				NULL,
				&virt_mem_data_port,
				&virt_mem_cfg,
				POST_KERNEL,
				CONFIG_MEM_INIT_PRIORITY,
				&virt_mem_api);
#endif /* CONFIG_VM_SHMEM */