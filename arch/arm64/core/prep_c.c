/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Full C support initialization
 *
 * Initialization of full C support: zero the .bss and call z_cstart().
 *
 * Stack is available in this module, but not the global data/bss until their
 * initialization is performed.
 */

#include <kernel_internal.h>
#include <linker/linker-defs.h>
#include <arch/arm64/debug_uart.h>

__weak void z_arm64_mm_init(bool is_primary_core) { }

extern FUNC_NORETURN void z_cstart(void);

extern void z_arm64_mm_init(bool is_primary_core);

static inline void z_arm64_bss_zero(void)
{
	uint64_t *p = (uint64_t *)__bss_start;
	uint64_t *end = (uint64_t *)__bss_end;

	while (p < end) {
		*p++ = 0U;
	}
}

/**
 *
 * @brief Prepare to and run C code
 *
 * This routine prepares for the execution of and runs C code.
 *
 * @return N/A
 */
void z_arm64_prep_c(void)
{
	/* init uart for output, for rk3568 */
#if defined(CONFIG_SOC_RK3568) && defined(CONFIG_ZVM_EARLYPRINT_MSG)
	uint32_t el;
	tpl_main();
	debug_printf("current_el: 0x%08x-%08x \r\n", (el>>32), el);
#endif

	/* Initialize tpidrro_el0 with our struct _cpu instance address */
	write_tpidrro_el0((uintptr_t)&_kernel.cpus[0]);
#if defined(CONFIG_HAS_ARM_VHE_EXTN) && defined(CONFIG_ZVM)
	arch_set_cpu_id_elx();
#endif
	z_arm64_bss_zero();
#ifdef CONFIG_XIP
	z_data_copy();
#endif
	z_arm64_mm_init(true);
	z_arm64_interrupt_init();
	z_cstart();

	CODE_UNREACHABLE;
}

#if CONFIG_MP_NUM_CPUS > 1
extern FUNC_NORETURN void z_arm64_secondary_start(void);
void z_arm64_secondary_prep_c(void)
{
	z_arm64_secondary_start();

	CODE_UNREACHABLE;
}
#endif
