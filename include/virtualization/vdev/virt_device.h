/*
 * Copyright 2023 HNU-ESNL
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

#define VM_DEVICE_INIT_RES      (0xFF)
#define VM_DEVICE_INVALID_BASE  (0xFFFFFFFF)
#define VM_DEVICE_INVALID_VIRQ  (0xFF)

/**
 * @brief DEVICE_TYPE for each vm, whcih is used to
 * judge whether it is nesscary to init when vm creat.
*/
#define VM_DEVICE_PRE_KERNEL_1  (0x01)

typedef void (*virt_device_irq_callback_user_data_set_t)(const struct device *dev,
                                    void *cb, void *user_data);

/**
 * @brief Get private date for vm.
*/
struct virt_device_data {
    /* virtual device types for vm, can be seen in macro 'DEVICE_TYPE'. */
    uint16_t vdevice_type;
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
    char device_type[VIRT_DEV_TYPE_LENGTH];
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
    const void *device_driver_api;
};

/**
 * @brief Virtual devices backend instance in zvm.
*/
struct virtual_device_instance {
    const char *name;
    struct virt_device_data *data;
    struct virt_device_config *cfg;
    const struct virt_device_api *api;
};

/* The overall virtual devices instances. */
extern const struct virtual_device_instance __virtual_device_instances_start[];
extern const struct virtual_device_instance __virtual_device_instances_end[];

/**
 * @brief Macro for creating a virtual device instance.
 *
 * @param _init_fn	Init function for virtual device.
 * @param _level    Init level.
 * @param _prio     Init priority.
 * @param _name		Name of the virtual device instance.
 * @param _data     Date of the virtual device instance.
 * @param _cfg      Configuration of virtual device.
 * @param _api		Virtual device backend API.
 * @param ...		Optional context.
 */
#define ZVM_VIRTUAL_DEVICE_DEFINE(_init_fn, _level, _prio, _name, _data, _cfg, _api, ...)	  \
    Z_INIT_ENTRY_DEFINE(Z_SYS_NAME(_init_fn), _init_fn, NULL, _level, _prio); \
	static const STRUCT_SECTION_ITERABLE(virtual_device_instance, _name) =	 \
	{								       \
        .name = STRINGIFY(_name),				\
        .data = &_data,                         \
        .cfg = &_cfg,                           \
		.api = &_api,						       \
	}

/**
 * @brief Get virtual device.
 *
 * @param[in] idx  Pointer to the virtual device instance.
 *
 * @return    Pointer to the virtual device instance.
 */
static inline const struct virtual_device_instance *zvm_virtual_device_get(uint32_t idx)
{
	return &__virtual_device_instances_start[idx];
}

/**
 * @brief Get number of virtual devices.
 *
 * @return Number of virtual devices.
*/
static inline int zvm_virtual_devices_count_get(void)
{
    return __virtual_device_instances_end - __virtual_device_instances_start;
}

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

/**
 * @brief Allocate device to vm, it will be called when device that will be
 * allocated to vm. Then, Set the device's irq for binding virt interrupt
 * with hardware interrupt.
 *
 * @return virt device instance.
*/
struct virt_dev *allocate_device_to_vm(const struct device *dev, struct vm *vm,
                        struct virt_dev *vdev_desc, bool pt_flag, bool shareable);

/**
 * @brief vm virt device call back function, which will be called when the device
 * that allocated to vm is triggerd.
*/
void vm_device_callback_func(const struct device *dev, void *cb, void *user_data);

/**
 * @brief create virtual device for each vm.
 *
 * @return virt device instance.
*/
struct virt_dev *create_vdevice_for_vm(const struct device *dev, struct vm *vm,
                        struct virt_dev *vdev_desc, bool pt_flag, bool shareable);

#endif /* ZEPHYR_INCLUDE_ZVM_VDEV_VIRT_DEVICE_H_ */
