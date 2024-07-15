/*
 * Copyright 2023 HNU-ESNL
 * Copyright 2023 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VDEV_SHMEM_H_
#define ZEPHYR_INCLUDE_ZVM_VDEV_SHMEM_H_

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <virtualization/vm.h>
#include <virtualization/vm_device.h>

/**
 * @brief shared memory devive config.
 */
struct shared_mem_config
{
	uint64_t base;
	uint64_t size;
};

/**
 * @brief virt memory device
 */
struct mem_vdevice {
	uint64_t mem_base;
	uint64_t mem_size;
};

/**
 * @brief init vm mem device for the vm.
 */
int vm_shmem_create(struct vm *vm);
int memory_read(struct virt_dev *vdev, uint64_t addr, uint64_t *value);
int memory_write(struct virt_dev *vdev, uint64_t addr, uint64_t *value);

#endif
