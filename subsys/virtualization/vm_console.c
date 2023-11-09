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
#include <virtualization/vm_mm.h>
#include <virtualization/vdev/virt_device.h>

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

    if (virq == VM_DEVICE_INVALID_VIRQ) {
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

int vm_console_create(struct vm *vm)
{
    bool chosen_flag=false;
    char *dev_name = VM_DEFAULT_CONSOLE_NAME;
    struct  _dnode *d_node, *ds_node;
    struct virt_dev *vm_dev, *chosen_dev = NULL;
    struct zvm_dev_lists* vdev_list;
    const struct device *dev;

    vdev_list = get_zvm_dev_lists();
    SYS_DLIST_FOR_EACH_NODE_SAFE(&vdev_list->dev_idle_list, d_node, ds_node){
        vm_dev = CONTAINER_OF(d_node, struct virt_dev, vdev_node);
        /* host uart device ? */
        if(!strncmp(dev_name, vm_dev->name, VM_DEFAULT_CONSOLE_NAME_LEN)){
            chosen_flag = true;
            break;
        }
    }

    if(chosen_flag){
        chosen_dev = vm_virt_dev_add(vm, "vm-console", true, false,
                            vm_dev->vm_vdev_paddr, VM_DEBUG_CONSOLE_BASE,
                            vm_dev->vm_vdev_size, vm_dev->hirq, VM_DEBUG_CONSOLE_IRQ);
        if(!chosen_dev){
            ZVM_LOG_WARN("there are no idle console uart for vm!");
            return -ENODEV;
        }
        /* move device to used node! */
        sys_dlist_remove(&vm_dev->vdev_node);
        sys_dlist_append(&vdev_list->dev_used_list, &vm_dev->vdev_node);

        vm_device_irq_init(vm, chosen_dev);

        dev = (struct device *)vm_dev->priv_data;
        vdev_irq_callback_user_data_set(dev, vm_uart_callback, chosen_dev);
    }
    return 0;
}
