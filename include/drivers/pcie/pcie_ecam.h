/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PCIE_PCIE_ECAM_H_
#define ZEPHYR_INCLUDE_DRIVERS_PCIE_PCIE_ECAM_H_

#include <kernel.h>
#include <device.h>
#include <drivers/pcie/pcie.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * PCIe Controllers Regions
 *
 * TOFIX:
 * - handle prefetchable regions
 */
enum pcie_region_type {
	PCIE_REGION_IO = 0,
	PCIE_REGION_MEM,
	PCIE_REGION_MEM64,
	PCIE_REGION_MAX,
};

struct pcie_ecam_data {
	uintptr_t cfg_phys_addr;
	mm_reg_t cfg_addr;
	size_t cfg_size;
	struct {
		uintptr_t phys_start;
		uintptr_t bus_start;
		size_t size;
		size_t allocation_offset;
	} regions[PCIE_REGION_MAX];
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_PCIE_PCIE_ECAM_H_ */
