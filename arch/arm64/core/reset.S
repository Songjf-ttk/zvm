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

_ASM_FILE_PROLOGUE

.macro z_rk3568_test_add_ops xreg0, xreg1, xreg2
	ldr \xreg0, =arm64_boot_counts
	ldr \xreg1, [\xreg0, #0]

	add \xreg1, \xreg1, \xreg2
	str \xreg1, [\xreg0, #0]
.endm

	.data
arm64_boot_counts:
	.word 0
	.global arm64_boot_counts

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
	/* Disable alignment fault checking */
	mrs	x0, sctlr_el2
	bic	x0, x0, SCTLR_A_BIT
	msr	sctlr_el2, x0

	/* Custom plat prep_c init, Default do nothing. */
	bl	z_arm64_el2_plat_prep_c

	/* Set SP_EL1, make the kernel stack bind to sp_el1 */
	msr		sp_el1, x24

#if	defined(CONFIG_ZVM) && defined(CONFIG_HAS_ARM_VHE_EXTN)
	msr		SPsel, #1
	mov     sp, x24
#endif
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
#if defined(CONFIG_SOC_RK3568)
	mov x1, x1, lsr #0x08
#endif
	cmp	x2, #-1
	beq	primary_core

#if defined(CONFIG_SOC_RK3568)
	msr SPsel, #1
	switch_el x13, 15f, 16f, 18f
18:
	nop
16:
	mov_imm x13, (SCTLR_EL2_RES1)
	msr sctlr_el2, x13
	isb
	mov_imm x13, (SPSR_MODE_EL2H | DAIF_FIQ_BIT)
	msr spsr_el2, x13
	isb

15:
	mov_imm x13, (SCTLR_EL1_RES1) /* Do not enable mmu */
	msr sctlr_el1, x13
	isb
	mov_imm x13, (SPSR_MODE_EL1H | DAIF_FIQ_BIT)
	msr spsr_el1, x13

#endif

	/* loop until our turn comes */
1:	dmb	ld
	ldr	x2, [x0, #BOOT_PARAM_MPID_OFFSET]
	cmp	x1, x2
	bne	1b
	/* rk3568 smp test pass this suit in 2023.06.02 */
	mrs x13, CurrentEL
	z_rk3568_test_add_ops x11, x12, x13

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
	mov_imm	x0, (SPSR_DAIF_MASK | SPSR_MODE_EL1T)
	msr	spsr_el2, x0

	/* Is VHE mode? */
	mrs x0,	hcr_el2
	and x0, x0,	#HCR_E2H_BIT
	cbz	x0,	21f
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
	/* Enable SError interrupts */
	msr	DAIFClr, #(DAIFCLR_ABT_BIT)
	isb

	ret	x25  /* either z_arm64_prep_c or z_arm64_secondary_prep_c */
