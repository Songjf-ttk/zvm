/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_PT_DEVICE_H_
#define ZEPHYR_INCLUDE_ZVM_PT_DEVICE_H_

typedef void (*ptdevice_irq_config_func_t)(const struct device *dev);
typedef void (*ptdevice_special_func_t)(void *user_data);

/* pass-through device description. */
struct pass_through_device_data {
    struct virt_dev *vdev;
};

struct pass_through_device_config {
    /*special init function.*/
    ptdevice_special_func_t	ptdev_spec_init_func;

    /*special irq function.*/
    ptdevice_special_func_t	ptdev_spec_irq_func;

    /*irq configuration function.*/
	ptdevice_irq_config_func_t	irq_config_func;
};

#endif /* ZEPHYR_INCLUDE_ZVM_PT_DEVICE_H_ */