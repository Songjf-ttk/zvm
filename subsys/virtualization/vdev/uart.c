/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/uart.h>

#include <virtualization/vm_console.h>
#include <virtualization/vm_dev.h>
#include <virtualization/zvm.h>
#include <virtualization/vdev/uart.h>

uint16_t zvm_uart_overall_id = 0;
static void zvm_vm_uart_irq_init(const struct device *dev, const struct virt_dev *dev_info)
{
    uart_irq_callback_user_data_set(dev, vm_uart_callback, (void*)dev_info);
}

int zvm_add_uart_dev(struct zvm_dev_lists *dev_list)
{
    struct virt_dev *dev_info;
    const struct device *dev;

#ifdef ZVM_VM_UART0_BASE
    dev_info = (struct virt_dev*)k_malloc(sizeof(struct virt_dev));
    if (dev_info == NULL) {
        return -ENOMEM;
    }

    dev_info->vm_vdev_paddr = ZVM_VM_UART0_BASE;
    dev_info->vm_vdev_size = ZVM_VM_UART0_SIZE;
    dev_info->vm_vdev_vaddr = get_vm_console_base();
    dev_info->hirq = ZVM_VM_UART0_IRQ;
    dev_info->virq = ZVM_VM_INVALID_UART_IRQ;

    dev_info->shareable = false;
    dev_info->dev_pt_flag = true;

    /* Init the dev's name */
    strcpy(dev_info->name, ZVM_VM_UART0_NAME);

    /* Get the overall id for this vm */
    dev_info->vdev_id = zvm_uart_overall_id++;

    /* init uart node */
    sys_dnode_init(&dev_info->vdev_node);

    /* add the node to dev list */
    sys_dlist_append(&dev_list->dev_idle_list, &dev_info->vdev_node);

    /* Get device info from macro. */
    dev = ZVM_VM_UART0_DEV;
    zvm_vm_uart_irq_init(dev, dev_info);

#endif /* ZVM_VM_UART0_BASE */

#ifdef ZVM_VM_UART1_BASE
    dev_info = (struct virt_dev*)k_malloc(sizeof(struct virt_dev));
    if (dev_info == NULL) {
        return -ENOMEM;
    }

    /* get the address info from dts */
    dev_info->vm_vdev_paddr = ZVM_VM_UART1_BASE;
    dev_info->vm_vdev_size = ZVM_VM_UART1_SIZE;
    dev_info->vm_vdev_vaddr = get_vm_console_base();
    dev_info->hirq = ZVM_VM_UART1_IRQ;
    dev_info->virq = ZVM_VM_INVALID_UART_IRQ;

    dev_info->shareable = false;
    dev_info->dev_pt_flag = true;
    strcpy(dev_info->name, ZVM_VM_UART1_NAME);
    dev_info->vdev_id = zvm_uart_overall_id++;
    sys_dnode_init(&dev_info->vdev_node);
    sys_dlist_append(&dev_list->dev_idle_list, &dev_info->vdev_node);

    dev = ZVM_VM_UART1_DEV;
    zvm_vm_uart_irq_init(dev, dev_info);
#endif /* ZVM_VM_UART1_BASE */

#ifdef ZVM_VM_UART2_BASE
    /* ...... */
#endif /* ZVM_VM_UART2_BASE */

    return 0;
}
