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
#include <virtualization/vdev/uart.h>
#include <virtualization/vm_mm.h>
#include <virtualization/arm/vgic_v3.h>
#include <virtualization/vdev/virt_device.h>

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

int vm_virt_dev_add(struct vm *vm, struct virt_dev *vm_dev, char *vdev_name,
            uint32_t pbase, uint32_t vbase, uint32_t size)
{
    int ret = 0;
    uint16_t name_len;

    vm_dev->vm_vdev_paddr = pbase;
    vm_dev->vm_vdev_vaddr = vbase;
    vm_dev->vm_vdev_size = size;

    sys_dlist_append(&vm->vdev_list, &vm_dev->vdev_node);
    if (!vdev_name) {
        sys_dlist_remove(&vm_dev->vdev_node);
        return -ENODEV;
    }
    ret = vm_vdev_mem_add(vm, vm_dev);
    name_len = strlen(vdev_name);
    name_len = name_len > VIRT_DEV_NAME_LENGTH ? VIRT_DEV_NAME_LENGTH : name_len;
    strncpy(vm_dev->name, vdev_name, name_len);
    vm_dev->name[name_len] = '\0';

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

static struct device_descriptor test_device_descs[] = {
	{   .device_addr_base  = 0x9000000,
        .device_addr_size = 0x1000,
        .device_irq = 0x21  },
};


int vm_vdevs_init(struct vm *vm)
{
    int ret;
    int i;
    uint8_t device_type;
    uint32_t dev_addr_base, dev_addr_size;
    struct virt_dev *vdev;
    const struct device *dev;
    struct device_descriptor *vdev_desc;

    sys_dlist_init(&vm->vdev_list);

    /* scan the dtb and get the device's node. */

    ret = vm_intctrl_vdev_create(vm);
    if (ret) {
        ZVM_LOG_WARN(" Init interrupt controller device error! \n");
		return -ENODEV;
    }

	vdev = (struct virt_dev *)k_malloc(sizeof(struct virt_dev));
	if (!vdev) {
        ZVM_LOG_ERR("Allocate memory for dgconsole error!\n");
        return -EMMAO;
    }

    /* find all the device in the system, test */
    for(i=0; i<1; i++){
        /* find the features that suit for different types  */
        dev_addr_base = test_device_descs[i].device_addr_base;
        if(dev_addr_base == 0x9000000){
            device_type = DEV_TYPE_PASSTHROUGH_DEVICE;
            dev = DEVICE_DT_GET(DT_ALIAS(vmserial1));
            vdev->vm_vdev_paddr = test_device_descs[i].device_addr_base;
            vdev->vm_vdev_size = test_device_descs[i].device_addr_size;
            vdev->virq = test_device_descs[i].device_irq;
            break;
        }
    }

    switch(device_type){
        case DEV_TYPE_EMULATE_ALL_DEVICE:
            break;
        case DEV_TYPE_VIRTIO_DEVICE:
            break;
        case DEV_TYPE_PASSTHROUGH_DEVICE:
            /** Address the suitable device init function
             * which will init the device and build the
             * memory map for vm.
            */
            ((const struct virt_device_api * const)(dev)->api)->init_fn(dev, vm, vdev);
            break;
        default:
            ZVM_LOG_WARN("VM's device type error!\n");
    }
/*
	ret = vm_monopoly_vdev_create(vm);
	if (ret) {
		ZVM_LOG_WARN(" Init monopoly device error! \n");
		return -ENODEV;
	}
*/
    return 0;
}
