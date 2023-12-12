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
#include <virtualization/vdev/virtio/virtio.h>

/* Magic value ("virt" string) - Read Only */
#define VIRTIO_MMIO_MAGIC_VALUE		0x000

/* Virtio device version - Read Only */
#define VIRTIO_MMIO_VERSION			0x004

/* Virtio device ID - Read Only */
#define VIRTIO_MMIO_DEVICE_ID		0x008

/* Virtio vendor ID - Read Only */
#define VIRTIO_MMIO_VENDOR_ID		0x00c

/* Bitmask of the features supported by the host (device)
 * (32 bits per set) - Read Only */
#define VIRTIO_MMIO_HOST_FEATURES		0x010
#define VIRTIO_MMIO_DEVICE_FEATURES		\
				VIRTIO_MMIO_HOST_FEATURES

/* Host (device) features set selector - Write Only */
#define VIRTIO_MMIO_HOST_FEATURES_SEL	0x014
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL	\
				VIRTIO_MMIO_HOST_FEATURES_SEL

/* Bitmask of features activated by the guest (driver)
 * (32 bits per set) - Write Only */
#define VIRTIO_MMIO_GUEST_FEATURES		0x020
#define VIRTIO_MMIO_DRIVER_FEATURES		\
				VIRTIO_MMIO_GUEST_FEATURES

/* Activated features set selector by the guest (driver) - Write Only */
#define VIRTIO_MMIO_GUEST_FEATURES_SEL	0x024
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL	\
				VIRTIO_MMIO_GUEST_FEATURES_SEL

/* Guest's memory page size in bytes - Write Only */
#define VIRTIO_MMIO_GUEST_PAGE_SIZE	0x028

/* Queue selector - Write Only */
#define VIRTIO_MMIO_QUEUE_SEL		0x030

/* Maximum size of the currently selected queue - Read Only */
#define VIRTIO_MMIO_QUEUE_NUM_MAX		0x034

/* Queue size for the currently selected queue - Write Only */
#define VIRTIO_MMIO_QUEUE_NUM		0x038

/* Used Ring alignment for the currently selected queue - Write Only */
#define VIRTIO_MMIO_QUEUE_ALIGN		0x03c

/* Guest's PFN for the currently selected queue - Read Write */
#define VIRTIO_MMIO_QUEUE_PFN		0x040

/* Ready bit for the currently selected queue - Read Write */
#define VIRTIO_MMIO_QUEUE_READY		0x044

/* Queue notifier - Write Only */
#define VIRTIO_MMIO_QUEUE_NOTIFY		0x050

/* Interrupt status - Read Only */
#define VIRTIO_MMIO_INTERRUPT_STATUS	0x060

/* Interrupt acknowledge - Write Only */
#define VIRTIO_MMIO_INTERRUPT_ACK		0x064

/* Device status register - Read Write */
#define VIRTIO_MMIO_STATUS			0x070

/* Selected queue's Descriptor Table address, 64 bits in two halves */
#define VIRTIO_MMIO_QUEUE_DESC_LOW		0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH		0x084

/* Selected queue's Available Ring address, 64 bits in two halves */
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW		0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH	0x094

/* Selected queue's Used Ring address, 64 bits in two halves */
#define VIRTIO_MMIO_QUEUE_USED_LOW		0x0a0
#define VIRTIO_MMIO_QUEUE_USED_HIGH		0x0a4

/* Configuration atomicity value */
#define VIRTIO_MMIO_CONFIG_GENERATION	0x0fc

/* The config space is defined by each driver as
 * the per-driver configuration space - Read Write */
#define VIRTIO_MMIO_CONFIG			0x100


/*
 * Interrupt flags (re: interrupt status & acknowledge registers)
 */

#define VIRTIO_MMIO_INT_VRING		(1 << 0)
#define VIRTIO_MMIO_INT_CONFIG		(1 << 1)

#define VIRTIO_MMIO_MAX_VQ			3
#define VIRTIO_MMIO_MAX_CONFIG		1
#define VIRTIO_MMIO_IO_SIZE			0x200

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
	int virtio_type;
#if defined(CONFIG_VIRTIO_INTERRUPT_DRIVEN)
    virtio_mmio_irq_config_func_t irq_config_func;
#endif
};

struct virtio_mmio_config {
	char    magic[4];
	uint32_t     version;
	uint32_t     device_id;
	uint32_t     vendor_id;
	uint32_t     host_features;
	uint32_t     host_features_sel;
	uint32_t     reserved_1[2];
	uint32_t     guest_features;
	uint32_t     guest_features_sel;
	uint32_t     guest_page_size;
	uint32_t     reserved_2;
	uint32_t     queue_sel;
	uint32_t     queue_num_max;
	uint32_t     queue_num;
	uint32_t     queue_align;
	uint32_t     queue_pfn;
    uint32_t     queue_read;
	uint32_t     reserved_3[2];
	uint32_t     queue_notify;
	uint32_t     reserved_4[3];
	uint32_t     interrupt_status;
	uint32_t     interrupt_ack;
	uint32_t     reserved_5[2];
	uint32_t     status;
} __attribute__((packed));

struct virtio_mmio_dev {
	struct vm *guest;
	struct virtio_device dev;
	struct virtio_mmio_config config;
	uint32_t irq;
};

/** @brief Driver API structure. */
__subsystem struct virtio_mmio_driver_api{
    int (*write) (struct virt_dev *edev,
			     uint32_t offset,
                 uint32_t src_mask,
                 uint32_t src,
                 uint32_t src_len);

	int (*read) (struct virt_dev *edev,
			    uint32_t offset,
                uint32_t *dst,
                uint32_t dst_len);

	int (*probe) (struct vm *guest,
		      struct virt_dev *edev);
	int (*remove) (struct virt_dev *edev);
	int (*reset) (struct virt_dev *edev);
};

#endif /* ZEPHYR_INCLUDE_ZVM_VDEV_VIRTIO_MMIO_H_ */