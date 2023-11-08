/*
 * Copyright 2023 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VDEV_VIRT_DEVICE_H_
#define ZEPHYR_INCLUDE_ZVM_VDEV_VIRT_DEVICE_H_

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <virtualization/vm.h>
#include <virtualization/vm_dev.h>

#define CONFIG_VIRT_DEVICE_INTERRUPT_DRIVEN 1

#define VM_DEVICE_INIT_RES      (0xFF)
#define VM_DEVICE_INVALID_BASE  (0xFFFFFFFF)
#define VM_DEVICE_INVALID_VIRQ  (0xFF)

typedef void (*virt_device_irq_callback_user_data_set_t)(const struct device *dev,
                                    void *cb, void *user_data);

struct virt_device_data {
    /* Get the virt device data port*/
    void *device_data;
#ifdef CONFIG_VIRT_DEVICE_INTERRUPT_DRIVEN
    virt_device_irq_callback_user_data_set_t irq_cb;
    void *irq_cb_data;
#endif
};

/**
 * @brief Get the device instance from dts, which include
 * the origin `device/config` from zephyr's device framwork.
*/
struct virt_device_config {
    /* Regisiter base and size from dts*/
    uint32_t reg_base;
    uint32_t reg_size;
    uint32_t hirq_num;
    /* Address of device instance config information */
    const void *device_config;
};

/**
 * @brief A virt device api for init or read/write device.
*/
struct virt_device_api {
    int (*init_fn)(const struct device *dev, struct vm *vm, struct virt_dev *vdev_desc);

    int (*virt_device_write)(struct virt_dev *vdev, uint64_t addr, uint64_t *value);
    int (*virt_device_read)(struct virt_dev *vdev, uint64_t addr, uint64_t *value);
#ifdef CONFIG_VIRT_DEVICE_INTERRUPT_DRIVEN
    void (*virt_irq_callback_set)(const struct device *dev, void *cb, void *user_data);
#endif
    /* Get the device driver api, if the device driver is initialed in host */
    void *device_driver_api;
};

/**
 * @brief Set the IRQ callback function pointer.
 *
 * This sets up the callback for IRQ. When an IRQ is triggered,
 * the specified function will be called with specified user data.
 *
 * @param dev virt device structure.
 * @param cb Pointer to the callback function.
 * @param user_data Data to pass to callback function.
 *
 * @return N/A
 */
static inline void vdev_irq_callback_user_data_set(const struct device *dev,
						   void *cb, void *user_data)
{
#ifdef CONFIG_VIRT_DEVICE_INTERRUPT_DRIVEN
	const struct virt_device_api *api =
		(const struct virt_device_api *)dev->api;

	if ((api != NULL) && (api->virt_irq_callback_set != NULL)) {
		api->virt_irq_callback_set(dev, cb, user_data);
	}
#endif
}

#endif /* ZEPHYR_INCLUDE_ZVM_VDEV_VIRT_DEVICE_H_ */
