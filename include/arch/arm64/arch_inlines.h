/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_ARCH_INLINES_H
#define ZEPHYR_INCLUDE_ARCH_ARM64_ARCH_INLINES_H

#ifndef _ASMLANGUAGE

#include <kernel_structs.h>
#include <arch/arm64/lib_helpers.h>
#include <arch/arm64/tpidrro_el0.h>

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
#ifndef CONFIG_ZVM
	return (_cpu_t *)(read_tpidrro_el0() & TPIDRROEL0_CURR_CPU);
#else
	return (_cpu_t *)(read_tpidr_el2() & TPIDRROEL0_CURR_CPU);
#endif
}

static ALWAYS_INLINE int arch_exception_depth(void)
{
	return (read_tpidrro_el0() & TPIDRROEL0_EXC_DEPTH) / TPIDRROEL0_EXC_UNIT;
}
/**
 * @brief set cpu id for zvm smp system
 *
 * @return ALWAYS_INLINE
 */
static ALWAYS_INLINE void arch_set_cpu_id_elx(void)
{
	write_tpidr_el2(read_tpidrro_el0());
}

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_ARCH_INLINES_H */
