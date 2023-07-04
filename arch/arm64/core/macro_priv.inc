/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_PRIV_INC_
#define _MACRO_PRIV_INC_

#include <arch/arm64/tpidrro_el0.h>

#ifdef _ASMLANGUAGE

/*
 * Get CPU id
 */

.macro get_cpu_id xreg0
	mrs	\xreg0, mpidr_el1
	/* FIMXME: aff3 not taken into consideration */
	ubfx	\xreg0, \xreg0, #0, #24
.endm

/*
 * Get CPU pointer
 */

.macro get_cpu xreg0
	mrs	\xreg0, tpidrro_el0
	and	\xreg0, \xreg0, #TPIDRROEL0_CURR_CPU
.endm

/*
 * Increment nested counter
 */

.macro inc_nest_counter xreg0, xreg1
	get_cpu	\xreg0
	ldr	\xreg1, [\xreg0, #___cpu_t_nested_OFFSET]
	add	\xreg1, \xreg1, #1
	str	\xreg1, [\xreg0, #___cpu_t_nested_OFFSET]
.endm

/*
 * Decrement nested counter and update condition flags
 */

.macro dec_nest_counter xreg0, xreg1
	get_cpu	\xreg0
	ldr	\xreg1, [\xreg0, #___cpu_t_nested_OFFSET]
	subs	\xreg1, \xreg1, #1
	str	\xreg1, [\xreg0, #___cpu_t_nested_OFFSET]
.endm


.macro	__dcache_line_size, reg, tmp
	mrs 	\tmp, ctr_el0
	ubfm	\tmp, \tmp, #16, #19	// cache line size encoding
	mov		\reg, #4		// bytes per word
	lsl		\reg, \reg, \tmp	// actual cache line size
.endm

.macro	adr_l, dst, sym
	adrp	\dst, \sym
	add	\dst, \dst, :lo12:\sym
.endm

.macro	ldr_l, dst, sym, tmp=
	.ifb	\tmp
	adrp	\dst, \sym
	ldr	\dst, [\dst, :lo12:\sym]
	.else
	adrp	\tmp, \sym
	ldr	\dst, [\tmp, :lo12:\sym]
	.endif
	.endm

/* smp related process for get current cpu context */
.macro this_cpu_address, dst, sym, tmp
    adrp    \tmp,   \sym
    add     \dst,   \tmp, #:lo12:\sym
    mrs     \tmp,   tpidr_el2
    add     \dst,   \dst, \tmp
.endm



#endif /* _ASMLANGUAGE */

#endif /* _MACRO_PRIV_INC_ */
