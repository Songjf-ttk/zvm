/*
 * Copyright 2019 Broadcom
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc.h>
#include <arch/arm64/arm_mmu.h>

#define SZ_1K	1024

static const struct arm_mmu_region mmu_regions[] = {

	MMU_REGION_FLAT_ENTRY("GIC",
			      DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 0),
			      DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic), 0),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE),

	MMU_REGION_FLAT_ENTRY("GIC",
			      DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 1),
			      DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic), 1),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE),

	MMU_REGION_FLAT_ENTRY("UART0",
			      DT_REG_ADDR(DT_INST(0, arm_pl011)),
			      DT_REG_SIZE(DT_INST(0, arm_pl011)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE),

#if defined(CONFIG_ZVM)
	MMU_REGION_FLAT_ENTRY("UART1",
			      DT_REG_ADDR(DT_INST(1, arm_pl011)),
			      DT_REG_SIZE(DT_INST(1, arm_pl011)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE),

	MMU_REGION_FLAT_ENTRY("UART2",
			      DT_REG_ADDR(DT_INST(2, arm_pl011)),
			      DT_REG_SIZE(DT_INST(2, arm_pl011)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE),

	MMU_REGION_FLAT_ENTRY("VM_SHMEM",
				  DT_REG_ADDR(DT_ALIAS(vmvirtmem)),
			      DT_REG_SIZE(DT_ALIAS(vmvirtmem)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE),

	MMU_REGION_FLAT_ENTRY("ZEPHYR_ELF_ADDR",
			      DT_REG_ADDR(DT_NODELABEL(zephyr_ddr)),
			      DT_REG_SIZE(DT_NODELABEL(zephyr_ddr)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_NA | MT_DEFAULT_SECURE_STATE),
#endif

};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
