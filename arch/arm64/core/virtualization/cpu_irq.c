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

#ifdef CONFIG_GIC_V3
#define VM_GIC_NAME			vm_gic_v3
#elif CONFIG_GIC_V2
#define VM_GIC_NAME			vm_gic_v2
#endif

bool arch_irq_ispending(struct vcpu *vcpu)
{
    uint32_t *mem_addr_base = NULL;
    uint32_t pend_addrend;
    struct vm *vm;
    struct virt_dev *vdev;
    struct  _dnode *d_node, *ds_node;

    vm = vcpu->vm;
    SYS_DLIST_FOR_EACH_NODE_SAFE(&vm->vdev_list, d_node, ds_node){
        vdev = CONTAINER_OF(d_node, struct virt_dev, vdev_node);
		if (!strcmp(vdev->name, TOSTRING(VM_GIC_NAME))) {
            mem_addr_base = arm_gic_get_distbase(vdev);
            break;
		}
    }

    if(mem_addr_base == NULL){
        ZVM_LOG_ERR("Can not find gic controller! \n");
        return false;
    }
    mem_addr_base += VGICD_ISPENDRn;
    pend_addrend = (uint64_t)mem_addr_base+(VGICD_ICPENDRn-VGICD_ISPENDRn);
    for(; (uint64_t)mem_addr_base < pend_addrend; mem_addr_base++){
        if(vgic_irq_test_bit(vcpu, 0, mem_addr_base, 32, 0)){
            return true;
        }
    }
    return false;
}