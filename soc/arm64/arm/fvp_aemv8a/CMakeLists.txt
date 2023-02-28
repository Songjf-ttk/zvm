# Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_ARM_MMU mmu_regions.c)


zephyr_library_sources_ifdef(CONFIG_SOC_FVP_BASE_REVC_2XAEMV8A plat_core.c)

zephyr_library_sources_ifdef(CONFIG_SOC_FVP_CORTEX_A55 plat_core.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FVP_CORTEX_A55 soc.c)