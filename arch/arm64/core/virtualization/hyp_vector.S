/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <toolchain.h>
#include <linker/sections.h>
#include <arch/arm64/tpidrro_el0.h>
#include <linker/sections.h>
#include <offsets.h>
#include <arch/cpu.h>

#include <virtualization/arm/asm.h>

_ASM_FILE_PROLOGUE

/*
 * Four types of exceptions:
 * - synchronous: aborts from MMU, SP/CP alignment checking, unallocated
 *   instructions, SVCs/SMCs/HVCs, ...)
 * - IRQ: group 1 (normal) interrupts
 * - FIQ: group 0 or secure interrupts
 * - SError: fatal system errors
 *
 * Four different contexts:
 * - from same exception level, when using the SP_EL0 stack pointer
 * - from same exception level, when using the SP_ELx stack pointer
 * - from lower exception level, when this is AArch64
 * - from lower exception level, when this is AArch32
 *
 * +------------------+------------------+-------------------------+
 * |     Address      |  Exception type  |       Description       |
 * +------------------+------------------+-------------------------+
 * | VBAR_ELn + 0x000 | Synchronous      | Current EL with SP0     |
 * |          + 0x080 | IRQ / vIRQ       |                         |
 * |          + 0x100 | FIQ / vFIQ       |                         |
 * |          + 0x180 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x200 | Synchronous      | Current EL with SPx     |
 * |          + 0x280 | IRQ / vIRQ       |                         |
 * |          + 0x300 | FIQ / vFIQ       |                         |
 * |          + 0x380 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x400 | Synchronous      | Lower EL using AArch64  |
 * |          + 0x480 | IRQ / vIRQ       |                         |
 * |          + 0x500 | FIQ / vFIQ       |                         |
 * |          + 0x580 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 * |          + 0x600 | Synchronous      | Lower EL using AArch32  |
 * |          + 0x680 | IRQ / vIRQ       |                         |
 * |          + 0x700 | FIQ / vFIQ       |                         |
 * |          + 0x780 | SError / vSError |                         |
 * +------------------+------------------+-------------------------+
 */

GDATA(_hyp_vector_table)
SECTION_SUBSEC_FUNC(hyp_vector_table,_vector_table_section,_hyp_vector_table)

	/* The whole table must be 2K aligned */
	.align 11

	/* Current EL with SP0 / Synchronous */
	.align 7
	b	.

	/* Current EL with SP0 / IRQ */
	.align 7
	b	.

	/* Current EL with SP0 / FIQ */
	.align 7
	b 	.

	/* Current EL with SP0 / SError */
	.align 7
	b	.

    /* Current EL with SPx / Synchronous */
	.align 7
	b	z_arm64_hyp_sync_exc

	/* Current EL with SPx / IRQ */
	.align 7
	b	z_arm64_hyp_irq_exc

	/* Current EL with SPx / FIQ */
	.align 7
	b 	.

	/* Current EL with SPx / SError */
	.align 7
	b	.

	/* Lower EL using AArch64 / Synchronous */
	.align 7
	b	z_arm64_hyp_sync_exc

	/* Lower EL using AArch64 / IRQ */
	.align 7
	b	z_arm64_hyp_irq_exc

	/* Lower EL using AArch64 / FIQ */
	.align 7
	b	.

	/* Lower EL using AArch64 / SError */
	.align 7
	b	.

	/* Lower EL using AArch32 / Synchronous */
	.align 7
	b	.

	/* Lower EL using AArch32 / IRQ */
	.align 7
	b	.

	/* Lower EL using AArch32 / FIQ */
	.align 7
	b	.

	/* Lower EL using AArch32 / SError */
	.align 7
	b	.

GTEXT(z_arm64_hyp_sync_exc)
SECTION_FUNC(TEXT, z_arm64_hyp_sync_exc)
	stp	x0, x1, [sp, #-16]!
	stp	x18, x30, [sp, #-16]!

	mrs x0, spsr_el2
	ubfx x1, x0, #0, #4

	cmp x1, #SPSR_MODE_EL2H
	b.eq current_el_handler

lower_el_handler:
	mrs x0, esr_el2
	bl z_vm_lower_sync_handler

	mov x1, #ARM_VM_EXCEPTION_SYNC
	b guest_vm_exit

current_el_handler:

	/* @TODO Error handling */
	b current_el_handler

GTEXT(z_arm64_hyp_irq_exc)
SECTION_FUNC(TEXT, z_arm64_hyp_irq_exc)
	stp	x0, x1, [sp, #-16]!
	stp	x18, x30, [sp, #-16]!

	mov x0, sp
	bl z_vm_lower_irq_handler

	mov x1, #ARM_VM_EXCEPTION_IRQ

	b guest_vm_exit
