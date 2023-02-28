/*
 * Copyright (c) 2021 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>

#ifdef CONFIG_SOC_QEMU_CORTEX_MAX

#ifndef CONFIG_PM_CPU_OPS_PSCI
int pm_cpu_on(unsigned long cpuid, uintptr_t entry_point)
{
	return 0;
}
#endif

#endif
