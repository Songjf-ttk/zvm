/*
 * Copyright 2021-2022 HNU-ESNL
 * Copyright 2023 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <drivers/uart.h>
#include <virtualization/vm_console.h>
#include <virtualization/vm_dev.h>
#include <virtualization/zvm.h>
#include <virtualization/vm_mm.h>
#include <virtualization/arm/vgic_v3.h>
#include <virtualization/vdev/virt_device.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

static int vm_vdev_mem_add(struct vm *vm, struct virt_dev *vdev)
{
    uint32_t attrs = 0;

    /*If device is emulated, set access off attrs*/
    if (vdev->dev_pt_flag) {
        attrs = MT_VM_DEVICE_MEM;
    }else{
        attrs = MT_VM_DEVICE_MEM | MT_S2_ACCESS_OFF;
    }

    return vm_vdev_mem_create(vm->vmem_domain, vdev->vm_vdev_paddr,
            vdev->vm_vdev_vaddr, vdev->vm_vdev_size, attrs);

}

struct virt_dev *vm_virt_dev_add(struct vm *vm, const char *dev_name, bool pt_flag,
                bool shareable, uint32_t dev_pbase, uint32_t dev_vbase,
                    uint32_t dev_size, uint32_t dev_hirq, uint32_t dev_virq)
{
    uint16_t name_len;
    int ret;
    struct virt_dev *vm_dev;

    vm_dev = (struct virt_dev *)k_malloc(sizeof(struct virt_dev));
	if (!vm_dev) {
        ZVM_LOG_ERR("Allocate memory for vm device error!\n");
        return NULL;
    }
    name_len = strlen(dev_name);
    name_len = name_len > VIRT_DEV_NAME_LENGTH ? VIRT_DEV_NAME_LENGTH : name_len;
    strncpy(vm_dev->name, dev_name, name_len);

    vm_dev->name[name_len] = '\0';
    vm_dev->dev_pt_flag = pt_flag;
    vm_dev->shareable = shareable;
    vm_dev->vm_vdev_paddr = dev_pbase;
    vm_dev->vm_vdev_vaddr = dev_vbase;
    vm_dev->vm_vdev_size = dev_size;

    ret = vm_vdev_mem_add(vm, vm_dev);
    if(ret){
        ZVM_LOG_ERR("Map vm device memory error!\n");
        return NULL;
    }
    vm_dev->virq = dev_virq;
    vm_dev->hirq = dev_hirq;
    vm_dev->vm = vm;
    sys_dlist_append(&vm->vdev_list, &vm_dev->vdev_node);

    return vm_dev;
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

int vm_device_init(struct vm *vm)
{
    int ret;

    sys_dlist_init(&vm->vdev_list);

    ret = vm_intctrl_vdev_create(vm);
    if (ret) {
        ZVM_LOG_WARN(" Init interrupt controller device error! \n");
		return -ENODEV;
    }

    ret = vm_console_create(vm);
	if (ret) {
        ZVM_LOG_WARN("Init vm debug console error! \n");
        return -EMMAO;
    }

    /* scan the dtb and get the device's node. */
    // switch(device_type){
    //     case DEV_TYPE_EMULATE_ALL_DEVICE:
    //         break;
    //     case DEV_TYPE_VIRTIO_DEVICE:
    //         break;
    //     case DEV_TYPE_PASSTHROUGH_DEVICE:
    //         /** Address the suitable device init function
    //          * which will init the device and build the
    //          * memory map for vm.
    //         */
    //         ((const struct virt_device_api * const)(dev)->api)->init_fn(dev, vm, vdev);
    //         break;
    //     default:
    //         ZVM_LOG_WARN("VM's device type error!\n");
    // }

    return 0;
}
