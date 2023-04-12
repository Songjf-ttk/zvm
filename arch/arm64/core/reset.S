/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include "boot.h"
#include "macro_priv.inc"


#define ASM_HCR_E2H		BIT(34)

_ASM_FILE_PROLOGUE

/*
 * Platform specific pre-C init code
 *
 * Note: - Stack is not yet available
 *       - x23, x24 and x25 must be preserved
 */

WTEXT(z_arm64_el3_plat_prep_c)
SECTION_FUNC(TEXT,z_arm64_el3_plat_prep_c)
	ret

WTEXT(z_arm64_el2_plat_prep_c)
SECTION_FUNC(TEXT,z_arm64_el2_plat_prep_c)
	ret

WTEXT(z_arm64_el1_plat_prep_c)
SECTION_FUNC(TEXT,z_arm64_el1_plat_prep_c)
	ret

/* clear dcache here */
GTEXT(rk3568_dcache_init)
SECTION_FUNC(TEXT,rk3568_dcache_init)
	add	x1, x1, x0

	mrs x3, ctr_el0
	ubfm x3, x3, #16, #19 	// cache line size encoding
	mov x2, #4
	lsl x2, x2, x3 

	sub	x3, x2, #1
	tst	x1, x3
	bic	x1, x1, x3
	b.eq	1f
	dc	civac, x1
1:	tst	x0, x3
	bic	x0, x0, x3
	b.eq	2f
	dc	civac, x0
	b	3f
2:	dc	ivac, x0
3:	add	x0, x0, x2
	cmp	x0, x1
	b.lo	2b
	dsb	sy
	ret

/*
 * Set the minimum necessary to safely call C code
 */

GTEXT(__reset_prep_c)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,__reset_prep_c)
	/* return address: x23 */
	mov	x23, x30

	switch_el x0, 3f, 2f, 1f
3:
	/* Reinitialize SCTLR from scratch in EL3 */
	ldr	w0, =(SCTLR_EL3_RES1 | SCTLR_I_BIT | SCTLR_SA_BIT)
	msr	sctlr_el3, x0

	/* Custom plat prep_c init */
	bl	z_arm64_el3_plat_prep_c

	/* Set SP_EL1 */
	msr     sp_el1, x24
#ifdef CONFIG_HAS_ARM_VHE_EXTN
	/* set SP_EL1 */
	msr		sp_el2, x24
#endif

	b	out
2:
<<<<<<< HEAD

=======
>>>>>>> 602ac10b (Arm64 core code: Add Arm64 core code for init zvm system)
#ifdef CONFIG_SOC_RK3568
	bl rk3568_dcache_init
#endif

	/* Disable alignment fault checking */
	mrs	x0, sctlr_el2
	bic	x0, x0, SCTLR_A_BIT
	msr	sctlr_el2, x0

	/* Custom plat prep_c init, Default do nothing. */
	bl	z_arm64_el2_plat_prep_c

	/* Set SP_EL1, make the kernel stack bind to sp_el1 */
	msr		sp_el1, x24
<<<<<<< HEAD

=======
>>>>>>> 602ac10b (Arm64 core code: Add Arm64 core code for init zvm system)
	msr		SPsel, #1
	mov     sp, x24

	b	out
1:
	/* Disable alignment fault checking */
	mrs	x0, sctlr_el1
	bic	x0, x0, SCTLR_A_BIT
	msr	sctlr_el1, x0

	/* Custom plat prep_c init */
	bl	z_arm64_el1_plat_prep_c

	/* Set SP_EL1. We cannot use sp_el1 at EL1 */
	msr     SPSel, #1
	mov     sp, x24
out:
	isb

	/* Select SP_EL0 , make all execption level use sp_el0*/
	msr	SPSel, #0

	/* Initialize stack */
	mov	sp, x24

	/* init uart here*/
	ret	x23

/*
 * Reset vector
 *
 * Ran when the system comes out of reset. The processor is in thread mode with
 * privileged level. At this point, neither SP_EL0 nor SP_ELx point to a valid
 * area in SRAM.
 */

GTEXT(__reset)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,__reset)

GTEXT(__start)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,__start)
	/* Mask all exceptions */
	msr	DAIFSet, #0xf

#if CONFIG_MP_NUM_CPUS > 1

	ldr	x0, =arm64_cpu_boot_params
	get_cpu_id x1
	ldr	x2, [x0, #BOOT_PARAM_MPID_OFFSET]
#ifdef CONFIG_BOARD_FVP_CORTEX_A55x4_A75x2_SMP
	mov x1, x1, lsr #0x08
	cmp x1, #0
	bne 1f
#endif
	cmp	x2, #-1
	beq	primary_core

	/* loop until our turn comes */
1:	dmb	ld
	ldr	x2, [x0, #BOOT_PARAM_MPID_OFFSET]
	cmp	x1, x2
	bne	1b

	/* we can now load our stack pointer value and move on */
	ldr	x24, [x0, #BOOT_PARAM_SP_OFFSET]
	ldr	x25, =z_arm64_secondary_prep_c
	b	2f

primary_core:
	/* advertise ourself */
	str	x1, [x0, #BOOT_PARAM_MPID_OFFSET]
#endif
	/* load primary stack and entry point */
	ldr	x24, =(z_interrupt_stacks + CONFIG_ISR_STACK_SIZE)
	ldr	x25, =z_arm64_prep_c
2:
	/* Prepare for calling C code */
	bl	__reset_prep_c

	/* Platform hook for highest EL */
	bl	z_arm64_el_highest_init

switch_el:
	switch_el x0, 3f, 2f, 1f
3:
	/* EL3 init */
	bl	z_arm64_el3_init

	/* Get next EL */
	adr	x0, switch_el
	bl	z_arm64_el3_get_next_el
	eret

2:
/* TODO: The init order may be change */
#if	defined(CONFIG_ZVM) && defined(CONFIG_HAS_ARM_VHE_EXTN)
	/* EL2 VHE mode init */
	bl	z_arm64_el2_vhe_init
#else
	bl  z_arm64_el2_init
#endif
<<<<<<< HEAD

=======
>>>>>>> 602ac10b (Arm64 core code: Add Arm64 core code for init zvm system)
	mov_imm	x0, (SPSR_DAIF_MASK | SPSR_MODE_EL1T)
	msr	spsr_el2, x0

#ifdef CONFIG_SOC_RK3568
	/* clean all cache here, JUST DO it on rk3568 */
	bl arch_flush_dcache_all
#endif

	/* Judge whether the cpu is support VHE */
	mrs x0,	hcr_el2
	and x0, x0,	#ASM_HCR_E2H
	/*If nvhe, jump! */
	cbz	x0,	21f
<<<<<<< HEAD

=======
>>>>>>> 602ac10b (Arm64 core code: Add Arm64 core code for init zvm system)
	b 1f

/* if vhe is not implemention */
21:
	adr	x0, 1f
	msr	elr_el2, x0
	eret

1:
	/* EL1 init */
	bl	z_arm64_el1_init

	/* We want to use SP_ELx from now on */
	msr	SPSel, #1
<<<<<<< HEAD

=======
>>>>>>> 602ac10b (Arm64 core code: Add Arm64 core code for init zvm system)
	/* Enable SError interrupts */
	msr	DAIFClr, #(DAIFCLR_ABT_BIT)
	isb

	ret	x25  /* either z_arm64_prep_c or z_arm64_secondary_prep_c */
