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

int vm_virq_block_desc_init(struct vm *vm, void *args)
{
    ARG_UNUSED(args);

    return vm_virq_desc_init(vm, NULL);
}

int vm_irq_ctlblock_init(struct vm *vm)
{
    return vm_irq_ctrlblock_create(NULL, vm);
}
