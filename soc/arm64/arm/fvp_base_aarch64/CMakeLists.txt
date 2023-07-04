# Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_ARM_MMU mmu_regions.c)

zephyr_library_sources_ifdef(CONFIG_SOC_FVP_BASE_A55X4_A75X2 plat_core.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FVP_BASE_A55X4_A75X2 soc.c)