/*
 * Copyright 2023 HNU-ESNL
 * Copyright 2023 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VDEV_VIRTIO_MMIO_H_
#define ZEPHYR_INCLUDE_ZVM_VDEV_VIRTIO_MMIO_H_

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/uart.h>
#include <virtualization/vm.h>

/**
 * @typedef virtio_mmio_irq_config_func_t
 * @brief For configuring IRQ on each individual virtio_mmio device.
 *
 * @param dev VirtIO MMIO device structure.
 *
 * @internal
 */
typedef void (*virtio_mmio_irq_config_func_t)(const struct device *dev);

/**
 * @brief VIRTIO_MMIO device configuration.
 */
struct virtio_device_config {
#if defined(CONFIG_VIRTIO_INTERRUPT_DRIVEN)
    virtio_mmio_irq_config_func_t irq_config_func;
#endif
};

/** @brief Driver API structure. */
__subsystem struct virtio_mmio_driver_api{

	/** Console I/O function */
	int (*err_check)(const struct device *dev);

	/** UART configuration functions */
	int (*configure)(const struct device *dev,
			 const struct uart_config *cfg);
	int (*config_get)(const struct device *dev, struct uart_config *cfg);
};

#endif /* ZEPHYR_INCLUDE_ZVM_VDEV_VIRTIO_MMIO_H_ */