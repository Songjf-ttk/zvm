/*
 * Copyright 2023 HNU-ESNL
 * Copyright 2023 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <zephyr.h>
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
#include <virtualization/vdev/shmem_rw.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define SHMEM_VIRQ (DT_IRQN(DT_ALIAS(vmvirtmem)) + VM_LOCAL_VIRQ_NR)

#define DEV_CFG(dev) ((const struct virt_device_config *const)(dev)->config)
#define DEV_DATA(dev) ((struct virt_device_data *)(dev)->data)


typedef struct shared_memory {
    int sender;         // sender ID
    int receiver;       // receiver ID
    int length;			// len of data
    char *shm_value;	// data
} SHMEM;

static struct virtual_device_instance *mem_instance;


/**
 * @brief Initialize virtual device's memory
*/
static int vm_virt_mem_rw_init(const struct device *dev, struct vm *vm, struct virt_dev *vdev_desc)
{
	struct virt_dev *vdev;
	struct mem_rw_vdevice *mem;
	struct zvm_dev_lists* vdev_list;
	struct  _dnode *d_node, *ds_node;
    struct virt_dev *vm_dev, *chosen_dev = NULL;
	char *dev_name = "VM_SHMEMRW";
	mem = (struct mem_rw_vdevice *)k_malloc(sizeof(struct mem_rw_vdevice));
    if (!mem) {
        ZVM_LOG_ERR("Allocate memory for mem error \n");
        return -ENODEV;
    }
	mem->mem_base = DT_REG_ADDR(DT_ALIAS(vmshmemrw));
	mem->mem_size = DT_REG_SIZE(DT_ALIAS(vmshmemrw));

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
								DT_REG_ADDR(DT_ALIAS(vmshmemrw)),
								DT_REG_ADDR(DT_ALIAS(vmshmemrw)),
								vm_dev->vm_vdev_size,
								VM_DEVICE_INVALID_VIRQ, SHMEM_VIRQ - 32);
		if(!vdev){
			ZVM_LOG_WARN("Init virt virt_mem_rw device error\n");
        	return -ENODEV;
		}
		vdev->priv_data = mem_instance;
		vdev->priv_vdev = dev;
        vm_device_irq_init(vm, vdev);
		ZVM_LOG_INFO("** Add %s device to vm successful. \n", dev->name);

		return 0;
	}
	return -ENODEV;
}

/**
 * @brief shmem irq
*/


/**
 * @brief Memory read operation
*/
int memory_rw_read(struct virt_dev *vdev, uint64_t addr, uint64_t *value)
{
	uint64_t *hva;
	uint64_t read_value;

	read_value = sys_read64(vdev->vm_vdev_paddr);
	printk("vm_vdev_paddr: 0x%x, read_value: 0x%x\n", vdev->vm_vdev_paddr, read_value);
	*(uint64_t *)value = read_value;
	return read_value;

}

/**
 * @brief Memory write operation
*/
int memory_rw_write(struct virt_dev *vdev, uint64_t addr, uint64_t *value)
{
	struct vm *vm;  // VM-zephyr
    char *char_value = (char *)value;

    // 打印 value 作为字符的内容
    printk("Value as characters: ");
    for (int i = 0; i < sizeof(uint64_t); i++) {
        printk("%c", char_value[i]);
    }
    printk("\n");
    /*
        '1' : irq from linux
        '2' : irq from linux (exit entry)
        '3' : irq from zephyr
    */
    if(char_value[0] == '1'){
        // linux sends irq to zephyr 
        vm = zvm_overall_info->vms[0];
        int ret = set_virq_to_vm(vm, SHMEM_VIRQ);
        if (ret < 0) {
                ZVM_LOG_WARN("Send virq to vm error!");
        }
        // printk("send irq_to_zephyr success!\n");
    } else if (char_value[0] == '2') {
        // linux send irq to zephyr
        vm = zvm_overall_info->vms[0];
        int ret2 = set_virq_to_vm(vm, SHMEM_VIRQ);
        // printk("send irq_to_linux success!\n");
    } else {
        // zephyr sends irq to linux
        vm = zvm_overall_info->vms[2];
        printk("vm_name : %s\n", vm->vm_name);
        int ret3 = set_virq_to_vm(vm, SHMEM_VIRQ);
    }
	return 0;
}

static const struct virt_device_api virt_mem_api = {
	.init_fn = vm_virt_mem_rw_init,
	.virt_device_write = memory_rw_write,
	.virt_device_read = memory_rw_read,
};

static int mem_rw_init(const struct device *dev)
{
	dev->state->init_res = VM_DEVICE_INIT_RES;
	return 0;
}

int vm_mem_rw_create(struct vm *vm)
{
	int ret = 0;
	const struct device *dev = DEVICE_DT_GET(DT_ALIAS(vmshmemrw));

	if(((const struct virt_device_api * const)(dev->api))->init_fn){
		((const struct virt_device_api * const)(dev->api))->init_fn(dev, vm, NULL);
	}else{
		ZVM_LOG_ERR("No mem_rw device api! \n");
		return -ENODEV;
	}
	return ret;
}


#ifdef CONFIG_VM_SHMEMRW
static struct shared_mem_rw_config mem_data_port = {
	.base = DT_REG_ADDR(DT_ALIAS(vmshmemrw)),
	.size = DT_REG_SIZE(DT_ALIAS(vmshmemrw)),
};

static struct virt_device_data virt_mem_data_port = {
	.device_data = &mem_data_port,
};

static struct virt_device_config virt_mem_cfg = {
	.reg_base = DT_REG_ADDR(DT_ALIAS(vmshmemrw)),
	.reg_size = DT_REG_SIZE(DT_ALIAS(vmshmemrw)),
	.device_config = &virt_mem_data_port,
};

/**
 * @brief Define the mem description for zvm.
*/
DEVICE_DT_DEFINE(DT_ALIAS(vmshmemrw),
				&mem_rw_init,
				NULL,
				&virt_mem_data_port,
				&virt_mem_cfg,
				POST_KERNEL,
				CONFIG_MEMRW_INIT_PRIORITY,
				&virt_mem_api);
#endif /* CONFIG_VM_SHMEMRW */