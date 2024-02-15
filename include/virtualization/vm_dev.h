/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VM_DEV_H_
#define ZEPHYR_INCLUDE_ZVM_VM_DEV_H_

#include <kernel.h>
#include <zephyr.h>
#include <device.h>
#include <kernel_structs.h>
#include <arch/cpu.h>
#include <virtualization/zvm.h>
#include <virtualization/arm/cpu.h>

#define VIRT_DEV_NAME_LENGTH        (32)
#define VIRT_DEV_TYPE_LENGTH        (32)

#define DEV_TYPE_EMULATE_ALL_DEVICE (0x01)
#define DEV_TYPE_VIRTIO_DEVICE      (0x02)
#define DEV_TYPE_PASSTHROUGH_DEVICE (0x03)

struct virt_dev {
    /* name of virtual device */
	char name[VIRT_DEV_NAME_LENGTH];

    /* Is this dev pass-through device? */
    bool dev_pt_flag;
    /* Is this dev virtio device?*/
    bool shareable;

    uint32_t hirq;
    uint32_t virq;

    uint32_t vm_vdev_paddr;
    uint32_t vm_vdev_vaddr;
	uint32_t vm_vdev_size;

	struct _dnode vdev_node;
	struct vm *vm;

    /**
     * Device private data may be usefull,
     * 1. For full virtual device, it store the emulated device private data.
     * 2. For passthrough device, it store the hardware instance data.
    */
    void *priv_data;

    /**
     * Binding to full virtual device driver.
    */
    void *priv_vdev;
};
typedef struct virt_dev virt_dev_t;

/**
 * @brief Save the overall idle dev list info.
 * Smp condition must be considered here.
 */
struct zvm_dev_lists {
    uint16_t dev_count;
    sys_dlist_t dev_idle_list;
    sys_dlist_t dev_used_list;
    /*TODO: Add smp lock here*/
};

/**
 * @brief According to the device info, create a vm device for the para @vm.
*/
struct virt_dev *vm_virt_dev_add(struct vm *vm, const char *dev_name, bool pt_flag,
                bool shareable, uint32_t dev_pbase, uint32_t dev_vbase,
                    uint32_t dev_size, uint32_t dev_hirq, uint32_t dev_virq);

/**
 * @brief write or read vdev for VM operation....
 */
int vdev_mmio_abort(arch_commom_regs_t *regs, int write, uint64_t addr, uint64_t *value, uint16_t size);

/**
 * @brief unmap passthrough device.
 */
int vm_unmap_ptdev(struct virt_dev *vdev, uint64_t vm_dev_base,
         uint64_t vm_dev_size, struct vm *vm);

int vm_vdev_pause(struct vcpu *vcpu);

/**
 * @brief Handle VM's device memory access. When @pa_addr is
 * located at a idle device, something need to do:
 * 1. Building a stage-2 translation table for this vm, which
 * can directly access this memory later.
 * 2. Rerun the fault code and access the physical device memory.
*/
int handle_vm_device_emulate(struct vm *vm, uint64_t pa_addr);

int vm_device_init(struct vm *vm);

#endif /* ZEPHYR_INCLUDE_ZVM_VM_DEV_H_ */
