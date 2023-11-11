/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ksched.h>
#include <spinlock.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>

#include <virtualization/arm/vgic_common.h>
#include <virtualization/arm/vgic_v3.h>
#include <virtualization/vm_irq.h>
#include <virtualization/zvm.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

void vm_device_irq_init(struct vm *vm, struct virt_dev *vm_dev)
{
    bool *bit_addr;
    struct virt_irq_desc *desc;

	/* Init virq desc for this irq */
	desc = get_virt_irq_desc(vm->vcpus[DEFAULT_VCPU], vm_dev->virq);
    desc->virq_flags |= VIRT_IRQ_HW;
    desc->id = VM_VIRT_CONSOLE_IRQ;
    desc->pirq_num = vm_dev->hirq;
    desc->virq_num = vm_dev->virq;

	/* set vm's uart irq bit */
	bit_addr = VGIC_DATA_IBP(vm->vm_irq_block_data);
	bit_addr[vm_dev->hirq] = true;

}

int vm_virq_block_desc_init(struct vm *vm, void *args)
{
    ARG_UNUSED(args);

    return vm_virq_desc_init(vm, NULL);
}

int vm_irq_ctlblock_init(struct vm *vm)
{
    return vm_irq_ctrlblock_create(NULL, vm);
}
