/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ksched.h>
#include <spinlock.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>
#include <virtualization/vdev/vgic_common.h>
#include <virtualization/vdev/vgic_v3.h>
#include <virtualization/vm_irq.h>
#include <virtualization/zvm.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

/**
 * @brief Init call for creating interrupt control block for vm.
 */
int vm_irq_ctrlblock_create(struct device *unused, struct vm *vm)
{
    ARG_UNUSED(unused);
	struct vm_virt_irq_block *vvi_block = &vm->vm_irq_block;

	if (VGIC_TYPER_LR_NUM != 0) {
		vvi_block->flags = 0;
		vvi_block->flags |= VIRQ_HW_SUPPORT;
	} else {
		ZVM_LOG_ERR("Init gicv3 failed, the hardware do not supporte it. \n");
		return -ENODEV;
	}

	vvi_block->enabled = false;
	vvi_block->cpu_num = CONFIG_MAX_VCPU_PER_VM;
	vvi_block->irq_num = VM_GLOBAL_VIRQ_NR;
	memset(vvi_block->ipi_vcpu_source, 0, sizeof(uint32_t)*CONFIG_MP_NUM_CPUS*VM_SGI_VIRQ_NR);
	memset(vvi_block->irq_bitmap, 0, VM_GLOBAL_VIRQ_NR/0x08);

	return 0;
}

/**
 * @brief Init virq descs for each vm. For It obtains some
 * device irq which is shared by all cores,
 * this type of interrupt is inited in this routine.
 */
static int vm_virq_desc_init(struct vm *vm)
{
    int i;
    struct virt_irq_desc *desc;

    for (i = 0; i < VM_SPI_VIRQ_NR; i++) {
		desc = &vm->vm_irq_block.vm_virt_irq_desc[i];

        desc->virq_flags = VIRQ_NOUSED_FLAG | VIRQ_HW_FLAG;
		/* For shared irq, it shared with all cores */
        desc->vcpu_id =  DEFAULT_VCPU;
        desc->vm_id = vm->vmid;
        desc->vdev_trigger = 0;
        desc->virq_num = i;
        desc->pirq_num = i;
        desc->id = VM_INVALID_DESC_ID;
        desc->virq_states = VIRQ_STATE_INVALID;
        desc->type = 0;

        sys_dnode_init(&(desc->desc_node));
    }

    return 0;
}

void vm_device_irq_init(struct vm *vm, struct virt_dev *vm_dev)
{
    bool *bit_addr;
    struct virt_irq_desc *desc;

	desc = get_virt_irq_desc(vm->vcpus[DEFAULT_VCPU], vm_dev->virq);
    if(vm_dev->dev_pt_flag){
        desc->virq_flags = VIRQ_NOUSED_FLAG | VIRQ_HW_FLAG;
    }else{
        ZVM_LOG_ERR("There is no supported virtual interrupt");
    }
    desc->id = desc->virq_num;
    desc->pirq_num = vm_dev->hirq;
    desc->virq_num = vm_dev->virq;

	bit_addr = vm->vm_irq_block.irq_bitmap;
	bit_addr[vm_dev->hirq] = true;
}

int vm_irq_block_init(struct vm *vm)
{
    int ret = 0;

    ret = vm_irq_ctrlblock_create(NULL, vm);
    if(ret){
        return ret;
    }
    ret = vm_virq_desc_init(vm);

    return ret;
}
