/*
 * Copyright 2024 HNU-ESNL
 * Copyright 2024 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_VIRTUALIZATION_VDEV_VIRT_PCI_H_
#define ZEPHYR_INCLUDE_VIRTUALIZATION_VDEV_VIRT_PCI_H_

#include <zephyr.h>
#include <kernel.h>
#include <devicetree.h>
#include <spinlock.h>

/* PCI device type. */
#define VM_PCI_BRIDGE   (0x01)
#define VM_PCI_DEVICE   (0x02)
#define VM_PCI_IVSHMEM  (0x03)

/* PCI shmem protocol type. */
#define VM_PCI_SHMEM_PROTO_UNDEFINED    (0x0000)
#define VM_PCI_SHMEM_PROTO_VETH		    (0x0001)
#define VM_PCI_SHMEM_PROTO_CUSTOM		(0x4000)	/* 0x4000..0x7fff */
#define VM_PCI_SHMEM_PROTO_VIRTIO_FRONT	(0x8000)	/* 0x8000..0xbfff */
#define VM_PCI_SHMEM_PROTO_VIRTIO_BACK	(0xc000)	/* 0xc000..0xffff */

/* Default is 4MB. */
#define VM_DEFAULT_PCIDEVICE_SIZE       (0x400000)

#define VM_PCI_RANGES_NUM   (5)

#define PCI_DEVICE_INIT     (1)
#define PCI_TRANSUCESS      (6)

/*Description of virtual pci bus device. */
struct virt_pci_bus_config {
    bool pci_is_virtual;
    uint16_t pci_domain;
    uint16_t pci_config_end_bus;

    /*cpu addree space for pci device.*/
    uint32_t pci_range_base[VM_PCI_RANGES_NUM];
    uint32_t pci_range_size[VM_PCI_RANGES_NUM];

};

/*Description of virtual pci private device. */
struct virt_pci_bus_data {
    /* host access pci bus's base address. */
    uint64_t pci_hva_base;
};

/* Description of virtual pci device. */
struct virt_pci_device {

    uint8_t type;
    uint8_t shmem_dev_id;
    uint8_t shmem_peers;
    uint8_t num_msi_vector;

    uint16_t domain;
    uint16_t bdf;
    uint16_t bar_mask[6];
    uint16_t caps_start;
    uint16_t num_caps;
    uint16_t shmem_protocol;

    uint32_t shmem_region_start;
};

/**
 * @brief Translation vm's access to physical memory.
 *
 * @return 0 is success, other value is wrong.
*/
int virt_pci_emulation(struct vm *vm, uint64_t pdev_addr, int write, uint64_t *value);

#endif /*ZEPHYR_INCLUDE_VIRTUALIZATION_VDEV_VIRT_PCI_H_*/