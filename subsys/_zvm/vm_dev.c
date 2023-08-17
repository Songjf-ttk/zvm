/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/uart.h>

#include <_zvm/vm_console.h>
#include <_zvm/vm_dev.h>
#include <_zvm/zvm.h>
#include <_zvm/vdev/uart.h>
#include <_zvm/vm_mm.h>
#include <_zvm/arm/vgic_v3.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);


static int vm_vdev_mem_add(struct vm *vm, struct virt_dev *vdev)
{
    uint32_t attrs = 0;

    if (vdev->dev_pt_flag) {
        attrs = MT_VM_DEVICE_MEM;
    }else{
        attrs = MT_VM_DEVICE_MEM | MT_S2_ACCESS_OFF;
    }

    return vm_vdev_mem_create(vm->vmem_domain, vdev->vm_vdev_paddr,
            vdev->vm_vdev_vaddr, vdev->vm_vdev_size, attrs);

}

int vm_virt_dev_add(struct vm *vm, struct virt_dev *vdev, char *vdev_name,
            uint32_t pbase, uint32_t vbase, uint32_t size)
{
    int ret = 0;
    uint16_t name_len;
    if (!vdev) {
        return -ENODEV;
    }

    /* get vm struct */
    vdev->vm = vm;
    vdev->vm_vdev_paddr = pbase;
    vdev->vm_vdev_vaddr = vbase;
    vdev->vm_vdev_size = size;

    sys_dlist_append(&vm->vdev_list, &vdev->vdev_node);
    if (!vdev_name) {
        sys_dlist_remove(&vdev->vdev_node);
        return -ENODEV;
    }
    ret = vm_vdev_mem_add(vm, vdev);

    name_len = strlen(vdev_name);
    name_len = name_len > VIRT_DEV_NAME_LENGTH ? VIRT_DEV_NAME_LENGTH : name_len;
    strncpy(vdev->name, vdev_name, name_len );
    vdev->name[name_len] = '\0';

    return ret;
}

int vdev_mmio_abort(arch_commom_regs_t *regs, int write, uint64_t addr,
                uint64_t *value)
{
    uint64_t *reg_value = value;
    struct vm *vm;
    struct virt_dev *vdev;
    struct  _dnode *d_node, *ds_node;

    vm = get_current_vm();

    SYS_DLIST_FOR_EACH_NODE_SAFE(&vm->vdev_list, d_node, ds_node){
        vdev = CONTAINER_OF(d_node, struct virt_dev, vdev_node);

        /* Find the addr on which vdev */
        if ((addr >= vdev->vm_vdev_paddr) && (addr < vdev->vm_vdev_paddr +
            vdev->vm_vdev_size)) {
            if (write) {
                return vdev->vm_vdev_write(vdev, regs, addr, reg_value);
            }else{
                return vdev->vm_vdev_read(vdev, regs, addr, reg_value);
            }
        }
    }

    /* Not found the vdev */
    ZVM_LOG_WARN("There are no virtual dev for this addr, addr : 0x%llx \n", addr);
    return -ENODEV;
}

int vm_monopoly_vdev_create(struct vm *vm)
{
    /* Init uart device */
    return vm_debug_console_add(vm);
}

int delete_vm_monopoly_vdev(struct vm *vm, struct virt_dev *vdev)
{
    /* remove uart device*/
    return vm_debug_console_remove(vm, vdev);
}

int vm_unmap_ptdev(struct virt_dev *vdev, uint64_t vm_dev_base,
         uint64_t vm_dev_size, struct vm *vm)
{
    int ret = 0;
    ARG_UNUSED(ret);
    uint64_t p_base, v_base, p_size, v_size;

    p_base = vdev->vm_vdev_paddr;
    p_size = vdev->vm_vdev_size;
    v_base = vm_dev_base;
    v_size = vm_dev_size;

    if (p_size != v_size || p_size == 0) {
        ZVM_LOG_WARN("The device is not matching, can not allocat this dev to the vm!");
        return -ENODEV;
    }

    return arch_vm_dev_domain_unmap(p_size, v_base, v_size, vdev->name, vm);

}

int vm_vdev_pause(struct vcpu *vcpu)
{
    ARG_UNUSED(vcpu);
    return 0;
}

int vm_vdevs_init(struct vm *vm)
{
    int ret;

    sys_dlist_init(&vm->vdev_list);

    ret = vm_intctrl_vdev_create(vm);
    if (ret) {
        ZVM_LOG_WARN(" Init interrupt controller device error! \n");
		return -ENODEV;
    }

    /*Init vm's monopoly device */
	ret = vm_monopoly_vdev_create(vm);
	if (ret) {
		ZVM_LOG_WARN(" Init monopoly device error! \n");
		return -ENODEV;
	}

    return 0;
}
