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

void vm_device_irq_init(struct vm *vm, struct virt_dev *vm_dev)
{
    bool *bit_addr;
    struct virt_irq_desc *desc;

	desc = get_virt_irq_desc(vm->vcpus[DEFAULT_VCPU], vm_dev->virq);
    desc->virq_flags |= VIRQ_FLAG_HW;
    desc->id = desc->virq_num % VM_VGIC_SUPPORT_REGS;
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
