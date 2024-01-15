/*
 * Copyright 2021-2022 HNU-ESNL
 * Copyright 2023 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <init.h>
#include <device.h>
#include <devicetree.h>
#include <virtualization/vdev/vgic_common.h>
#include <virtualization/vdev/virt_device.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

/**
 * A virt device sample that for zvm system.
 *
 * @brief Define the serial1 description for zvm, the sample:
 *
 * @param DT_ALIAS(serial1): node identifier for this device;
 *
 * @param &serial1_init: init function for zvm;
 *
 * @param NULL: pm_device, which usually not used;
 *
 * @param &serial1_data_port_1: serial config data, which is not hardware device related data;
 *
 * @param &virt_serial1_cfg: serial config data, which is hardware device related data;
 *
 * @param POST_KERNEL: init post kernel level;
 *
 * @param CONFIG_SERIAL_INIT_PRIORITY: init priority
 *
 * @param &serial_driver_api: serial driver api;
*/
/*
DEVICE_DT_DEFINE(DT_ALIAS(serial1),
            &serial_init,
            NULL,
            &serial1_data_port_1,
            &virt_serial1_cfg, POST_KERNEL,
            CONFIG_SERIAL_INIT_PRIORITY,
            &serial_driver_api);
*/

#define DEV_CFG(dev) \
	((const struct virt_device_config * const)(dev)->config)
#define DEV_DATA(dev) \
	((struct virt_device_data *)(dev)->data)

struct virt_dev *allocate_device_to_vm(const struct device *dev, struct vm *vm,
                        struct virt_dev *vdev_desc, bool pt_flag, bool shareable)
{
    struct virt_dev *vdev;

    vdev = vm_virt_dev_add(vm, dev->name, true, false, DEV_CFG(dev)->reg_base,
					vdev_desc->vm_vdev_paddr, DEV_CFG(dev)->reg_size,
					DEV_CFG(dev)->hirq_num, vdev_desc->virq);
    if(!vdev){
        return NULL;
    }

    vm_device_irq_init(vm, vdev);
    return vdev;
}

void vm_device_callback_func(const struct device *dev, void *cb,
                void *user_data)
{
    uint32_t virq;
    ARG_UNUSED(cb);
    int err = 0;
    const struct virt_dev *vdev = (const struct virt_dev *)user_data;

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
