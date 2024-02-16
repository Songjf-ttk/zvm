/*
 * Copyright 2022-2023 HNU-ESNL
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <kernel.h>
#include <virtualization/vm.h>
#include <virtualization/vdev/vgic_common.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

bool arch_irq_ispending(struct vcpu *vcpu)
{
    uint32_t *mem_addr_base = NULL;
    struct vm *vm;
    struct virt_dev *vdev;
    struct  _dnode *d_node, *ds_node;

    vm = vcpu->vm;

    SYS_DLIST_FOR_EACH_NODE_SAFE(&vm->vdev_list, d_node, ds_node){
        vdev = CONTAINER_OF(d_node, struct virt_dev, vdev_node);
		if (!strcmp(vdev->name, "VM_VGIC")) {
            mem_addr_base = &vdev->vm_vdev_vaddr;
            break;
		}
    }

    if(mem_addr_base == NULL){
        ZVM_LOG_ERR("Can not find gic controller! \n");
        return false;
    }
    return vgic_irq_test_bit(vcpu, 0, mem_addr_base, 32, 0);
}