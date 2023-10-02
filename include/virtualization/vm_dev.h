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
#include <virtualization/vdev/uart.h>

#define VIRT_DEV_NAME_LENGTH    (32)

struct virt_dev;

typedef int (*vm_vdev_write_t)(struct virt_dev *vdev, arch_commom_regs_t *regs, uint64_t addr, uint64_t *value);
typedef int (*vm_vdev_read_t)(struct virt_dev *vdev, arch_commom_regs_t *regs, uint64_t addr, uint64_t *value);

/**
 * @brief virtual device struct.
 */
struct virt_dev {
	char name[VIRT_DEV_NAME_LENGTH];
    bool shareable;

    /* Is this dev pass through */
    bool dev_pt_flag;

    uint16_t vdev_id;
    uint32_t hirq;
    uint32_t virq;

    uint32_t vm_vdev_paddr;
    uint32_t vm_vdev_vaddr;
	uint32_t vm_vdev_size;

	struct _dnode vdev_node;
	struct vm *vm;

    vm_vdev_write_t vm_vdev_write;
    vm_vdev_read_t  vm_vdev_read;
};
typedef struct virt_dev virt_dev_t;

/**
 * @brief Save the overall idle dev list info.
 * Smp condition must be considered here.
 */
struct zvm_dev_lists{
    uint16_t dev_count;
    sys_dlist_t dev_idle_list;
    sys_dlist_t dev_used_list;
    /*TODO: Add smp lock here*/
};


int vm_virt_dev_add(struct vm *vm, struct virt_dev *vdev, char *vdev_name, uint32_t pbase,
                        uint32_t vbase, uint32_t size);

/**
 * @brief write or read vdev for VM operation....
 */
int vdev_mmio_abort(arch_commom_regs_t *regs, int write, uint64_t addr, uint64_t *value);

/**
 * @brief unmap passthrough device.
 */
int vm_unmap_ptdev(struct virt_dev *vdev, uint64_t vm_dev_base,
         uint64_t vm_dev_size, struct vm *vm);

int vm_monopoly_vdev_create(struct vm *vm);
int delete_vm_monopoly_vdev(struct vm *vm, struct virt_dev *dev_info);

int vm_vdev_pause(struct vcpu *vcpu);
int vm_vdevs_init(struct vm *vm);

#endif /* ZEPHYR_INCLUDE_ZVM_VM_DEV_H_ */
