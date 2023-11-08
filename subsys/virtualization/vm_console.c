/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <drivers/uart.h>
#include <kernel_arch_interface.h>
#include <virtualization/vm_console.h>
#include <virtualization/zvm.h>
#include <virtualization/arm/mm.h>
#include <virtualization/arm/vgic_v3.h>
#include <virtualization/arm/vgic_common.h>
#include <virtualization/vdev/uart.h>
#include <virtualization/vm_mm.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

void vm_uart_callback(const struct device *dev, void *cb,
                void *user_data)
{
    uint32_t virq, pirq;
    ARG_UNUSED(cb);
    ARG_UNUSED(pirq);
    int err = 0;
    const struct virt_dev *vdev = (const struct virt_dev *)user_data;

    uart_irq_update(dev);
    virq = vdev->virq;

    if (virq == ZVM_VM_INVALID_UART_IRQ) {
        ZVM_LOG_WARN("Invalid interrupt occur! \n");
        return;
    }
    if (!vdev->vm) {
        ZVM_LOG_WARN("VM struct not exit here!");
        return;
    }

    err = set_virq_to_vm(vdev->vm, virq);
    if (err < 0) {
        ZVM_LOG_WARN("Send virq to vm error!");
    }
}

/**
 * @brief Mem map the debug console address to the idle device.
 *  mem map it to the VM's uart address space.
 *
 * @param vm
 * @return int
 */
static int mmap_debug_console(struct virt_dev *vdev, struct vm *vm)
{
    char *name;
    uint64_t p_base, v_base, p_size, v_size;

    /* Get the default console address */
    p_base = vdev->vm_vdev_paddr;
    p_size = vdev->vm_vdev_size;
    v_base = get_vm_console_base();
    v_size = get_vm_console_size();

    name = vdev->name;
    if (p_size != v_size || p_size == 0) {
        ZVM_LOG_WARN("The device is not matching, can not allocat \
this dev to the vm!");
        return -ENOVDEV;
    }

    return arch_vm_dev_domain_map(p_base, v_base, p_size, name, vm);
}

static int unmap_debug_console(struct virt_dev *vdev, struct vm *vm)
{
    int ret = 0;
    ARG_UNUSED(ret);
    uint64_t vm_dev_base, vm_dev_size;

    vm_dev_base = get_vm_console_base();
    vm_dev_size = get_vm_console_size();

    return vm_unmap_ptdev(vdev, vm_dev_base, vm_dev_size, vm);
}

int vm_debug_console_add(struct vm *vm)
{
    int ret;
    struct virt_dev *dbgcon_dev = NULL;
    struct virt_irq_desc *desc;
    struct virt_dev *vdev = NULL;
    struct zvm_dev_lists *dev_lists;
    struct  _dnode *d_node, *ds_node;

    dev_lists = get_zvm_dev_lists();
    SYS_DLIST_FOR_EACH_NODE_SAFE(&dev_lists->dev_idle_list, d_node, ds_node){
        vdev = CONTAINER_OF(d_node, struct virt_dev, vdev_node);
        if (strncmp(vdev->name, ZVM_VM_UART0_NAME, 4)) {
            continue;
        }
        break;
    }

    if (vdev == NULL) {
        ZVM_LOG_WARN("There is no idle zvm device here!");
        return -ENOVDEV;
    }

    sys_dlist_remove(&vdev->vdev_node);
    sys_dlist_append(&dev_lists->dev_used_list, &vdev->vdev_node);

    /* This vdev need to bind to VM */
    dbgcon_dev = (struct virt_dev *)k_malloc(sizeof(struct virt_dev));
    if (!dbgcon_dev) {
        ZVM_LOG_ERR("Allocate memory for dgconsole error!\n");
        return -EMMAO;
    }

    dbgcon_dev->dev_pt_flag = vdev->dev_pt_flag;
    dbgcon_dev->shareable = vdev->shareable;
    ret = vm_virt_dev_add(vm, dbgcon_dev, vdev->name, vdev->vm_vdev_paddr,
        vdev->vm_vdev_vaddr, vdev->vm_vdev_size);
    if (ret) {
        ZVM_LOG_WARN("Init virt dev error\n");
        return -ENOVDEV;
    }
    dbgcon_dev->hirq = vdev->hirq;
    dbgcon_dev->virq = VM_DEFAULT_CONSOLE_IRQ;

    ret = mmap_debug_console(dbgcon_dev, vm);
    if (ret) {
        return -ENOVDEV;
    }

    /* We should find the default irq info from VM'dts later @TODO */
    vdev->virq = dbgcon_dev->virq;
    /* Bind the vdev to vm. */
    vdev->vm = vm;

    /* Find virq desc for this irq */
    desc = get_virt_irq_desc(vm->vcpus[DEFAULT_VCPU], vdev->virq);
    desc->virq_flags |= VIRT_IRQ_HW;
    desc->id = VM_VIRT_CONSOLE_IRQ;
    desc->pirq_num = vdev->hirq;
    desc->virq_num = vdev->virq;

    /* set vm's uart virq bit map */
    bool *bit_addr = VGIC_DATA_IBP(vm->vm_irq_block_data);
    bit_addr[vdev->hirq] = true;

    return 0;
}

int vm_debug_console_remove(struct vm *vm, struct virt_dev *vdev_remove)
{
    int ret;
    uint32_t irq;
    struct zvm_dev_lists *dev_lists;
    struct  _dnode *d_node, *ds_node;
    struct virt_dev *vdev;

    dev_lists = get_zvm_dev_lists();
    SYS_DLIST_FOR_EACH_NODE_SAFE(&dev_lists->dev_used_list, d_node, ds_node){

        /* Find the idle dev */
        vdev = CONTAINER_OF(d_node, struct virt_dev, vdev_node);

        if (vdev->hirq == vdev_remove->hirq) {
            sys_dlist_remove(&vdev->vdev_node);
            sys_dlist_append(&dev_lists->dev_idle_list, &vdev->vdev_node);
            break;
        }

    }

    /* disable spi interrupt */
    irq = vdev_remove->hirq;
    if (irq <= CONFIG_NUM_IRQS) {
        arm_gic_irq_disable(irq);
    }

    /* Release the vdev from vm. */
    vdev->vm = NULL;

    /* unmap to the system map  */
    ret = unmap_debug_console(vdev_remove, vm);
    if (ret) {
        return -ENOVDEV;
    }
    /* free the virt dev for this vm */
    k_free(vdev_remove);

    return ret;
}
