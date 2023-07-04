/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <soc.h>
#include <arch/arm64/arm_mmu.h>

static const struct arm_mmu_region mmu_regions[] = {

	MMU_REGION_FLAT_ENTRY("GIC",
			      DT_REG_ADDR_BY_IDX(DT_NODELABEL(gic), 0),
			      DT_REG_SIZE_BY_IDX(DT_NODELABEL(gic), 0),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),

	MMU_REGION_FLAT_ENTRY("GIC",
			      DT_REG_ADDR_BY_IDX(DT_NODELABEL(gic), 1),
			      DT_REG_SIZE_BY_IDX(DT_NODELABEL(gic), 1),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),

	MMU_REGION_FLAT_ENTRY("CRU",
			      DT_REG_ADDR(DT_NODELABEL(cru)),
			      DT_REG_SIZE(DT_NODELABEL(cru)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),

	MMU_REGION_FLAT_ENTRY("UART2",
			      DT_REG_ADDR(DT_NODELABEL(uart2)),
			      DT_REG_SIZE(DT_NODELABEL(uart2)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),
	/* TODO: Add more device entry */
/*
	MMU_REGION_FLAT_ENTRY("PER_DEVICE",
			      DT_REG_ADDR(DT_NODELABEL(per_device)),
			      DT_REG_SIZE(DT_NODELABEL(per_device)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),

	MMU_REGION_FLAT_ENTRY("PCIE_DEVICE",
			      DT_REG_ADDR(DT_NODELABEL(pcie_device)),
			      DT_REG_SIZE(DT_NODELABEL(pcie_device)),
			      MT_DEVICE_nGnRnE | MT_P_RW_U_RW | MT_NS),
*/
};

const struct arm_mmu_config mmu_config = {
	.num_regions = ARRAY_SIZE(mmu_regions),
	.mmu_regions = mmu_regions,
};
