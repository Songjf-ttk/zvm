#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <syscall.h>
#include "../core/macro_priv.inc"


_ASM_FILE_PROLOGUE

GDATA(BootromContex)
	.align 10
	.zero 8*64
	.align 10

GTEXT(safe_print_debug)
SECTION_FUNC(TEXT, safe_print_debug)

	//ldr x0, =BootromContex
	//ldr  x0, [sp]
	sub	sp, sp, #0x0100			//256 byte
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	stp	x16, x17, [sp, #16 * 8]
	stp	x18, x19, [sp, #16 * 9]
	stp	x20, x21, [sp, #16 * 10]
	stp	x22, x23, [sp, #16 * 11]
	stp	x24, x25, [sp, #16 * 12]
	stp	x26, x27, [sp, #16 * 13]
	stp	x28, x29, [sp, #16 * 14]
	stp	x30, xzr, [sp, #16 * 15]

	bl  early_print_debug

	ldp	x0, x1, [sp, #16 * 0]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x14, x15, [sp, #16 * 7]
	ldp	x16, x17, [sp, #16 * 8]
	ldp	x18, x19, [sp, #16 * 9]
	ldp	x20, x21, [sp, #16 * 10]
	ldp	x22, x23, [sp, #16 * 11]
	ldp	x24, x25, [sp, #16 * 12]
	ldp	x26, x27, [sp, #16 * 13]
	ldp	x28, x29, [sp, #16 * 14]
	ldp	x30, xzr, [sp, #16 * 15]
	add	sp, sp, #0x0100

	ret

GTEXT(safe_print_debug_smp)
SECTION_FUNC(TEXT, safe_print_debug_smp)

	sub	sp, sp, #0x0100			//256 byte
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	stp	x16, x17, [sp, #16 * 8]
	stp	x18, x19, [sp, #16 * 9]
	stp	x20, x21, [sp, #16 * 10]
	stp	x22, x23, [sp, #16 * 11]
	stp	x24, x25, [sp, #16 * 12]
	stp	x26, x27, [sp, #16 * 13]
	stp	x28, x29, [sp, #16 * 14]
	stp	x30, xzr, [sp, #16 * 15]

	bl  asm_register_print_smp

	ldp	x0, x1, [sp, #16 * 0]
	ldp	x2, x3, [sp, #16 * 1]
	ldp	x4, x5, [sp, #16 * 2]
	ldp	x6, x7, [sp, #16 * 3]
	ldp	x8, x9, [sp, #16 * 4]
	ldp	x10, x11, [sp, #16 * 5]
	ldp	x12, x13, [sp, #16 * 6]
	ldp	x14, x15, [sp, #16 * 7]
	ldp	x16, x17, [sp, #16 * 8]
	ldp	x18, x19, [sp, #16 * 9]
	ldp	x20, x21, [sp, #16 * 10]
	ldp	x22, x23, [sp, #16 * 11]
	ldp	x24, x25, [sp, #16 * 12]
	ldp	x26, x27, [sp, #16 * 13]
	ldp	x28, x29, [sp, #16 * 14]
	ldp	x30, xzr, [sp, #16 * 15]
	add	sp, sp, #0x0100

	ret



GTEXT(setjmp)
SECTION_FUNC(TEXT, setjmp)
	/* 保存上下文 */
	stp  x19, x20, [x0,#0]
	stp  x21, x22, [x0,#16]
	stp  x23, x24, [x0,#32]
	stp  x25, x26, [x0,#48]
	stp  x27, x28, [x0,#64]
	stp  x29, x30, [x0,#80]
	mov  x2, sp
	str  x2, [x0, #96]
	mov  x0, #0
	ret

GTEXT(longjmp)
SECTION_FUNC(TEXT, longjmp)
	ldp  x19, x20, [x0,#0]
	ldp  x21, x22, [x0,#16]
	ldp  x23, x24, [x0,#32]
	ldp  x25, x26, [x0,#48]
	ldp  x27, x28, [x0,#64]
	ldp  x29, x30, [x0,#80]
	ldr  x2, [x0,#96]
	mov  sp, x2
	/* 恢复上下文, 第二个参数做返回值，如果第二个参数是0就加1返回1  */
	adds x0, xzr, x1
	csinc x0, x0, xzr, ne
	/* invalid icache for cortex a35 */
	//branch_if_a35_core x1, __asm_invalidate_icache_all
	ret


// Define some constants
.equ UART_BASE, 0xfdcc0000 // Base address of uart port
.equ UART_THR, 0x00 // Transmit holding register offset
.equ UART_LSR, 0x14 // Line status register offset


GTEXT(uart_putc)
SECTION_FUNC(TEXT, uart_putc)
    ldr x1, =UART_BASE // Load base address of uart port to x1
    ldrb w2, [x0], #1 // Load a byte from string to w2 and increment x0 by 1
    cbz w2, uart_putc_done // If w2 is zero, branch to putc_done
uart_putc_loop:
    ldrb w3, [x1, UART_LSR] // Load line status register to w3
    tbz w3, #5, uart_putc_loop // If transmit holding register empty bit is not set, loop
    strb w2, [x1, UART_THR] // Store w2 to transmit holding register
    b uart_putc // Branch to putc
uart_putc_done:
    ret // Return from subroutine

GTEXT(uart_putn)
SECTION_FUNC(TEXT, uart_putn)
    mov x1, #10 // Load 10 to x1 as the base of decimal system
    mov x2, sp // Load stack pointer to x2 as the buffer pointer
uart_putn_loop:
    udiv x3, x0, x1 // Divide x0 by x1 and store the quotient in x3
    msub x4, x3, x1, x0 // Multiply x3 by x1 and subtract from x0 and store the remainder in x4
    add x4, x4, #'0' // Add '0' to x4 to convert it to a character
    strb w4, [x2], #-1 // Store w4 to the buffer and decrement x2 by 1
    cbz x3, uart_putn_done // If x3 is zero, branch to putn_done
    mov x0, x3 // Move x3 to x0 as the new number to output
    b uart_putn_loop // Branch to putn_loop
uart_putn_done:
    add x0, x2, #1 // Add 1 to x2 and store it in x0 as the string pointer
    bl uart_putc // Branch with link to putc subroutine
    ret // Return from subroutine


GTEXT(asm_safe_uart_output)
SECTION_FUNC(TEXT, asm_safe_uart_output)

    stp x29,x28,[sp,#-16]!// Store x29 and x28 pair on the stack and decrement sp by 16 
    stp x27,x26,[sp,#-16]!// Store x27 and x26 pair on the stack and decrement sp by 16 
    stp x25,x24,[sp,#-16]!// Store x25 and x24 pair on the stack and decrement sp by 16 
    stp x23,x22,[sp,#-16]!// Store x23 and x22 pair on the stack and decrement sp by 16 
    stp x21,x20,[sp,#-16]!// Store x21 and x20 pair on the stack and decrement sp by 16 
    stp x19,x18,[sp,#-16]!// Store x19 and x18 pair on the stack and decrement sp by 16 
	stp x17,x16,[sp,#-16]!// Store x19 and x18 pair on the stack and decrement sp by 16
	stp x15,x14,[sp,#-16]!// Store x15 and x14 pair on the stack and decrement sp by 16 
    stp x13,x12,[sp,#-16]!// Store x13 and x12 pair on the stack and decrement sp by 16 
    stp x11,x10,[sp,#-16]!// Store x11 and x10 pair on the stack and decrement sp by 16 
    stp x9,x8,[sp,#-16]!// Store x9 and x8 pair on the stack and decrement sp by 16 
    stp x7,x6,[sp,#-16]!// Store x7 and x6 pair on the stack and decrement sp by 16 
    stp x5,x4,[sp,#-16]!// Store x5 and x4 pair on the stack and decrement sp by 16 
    stp x3,x2,[sp,#-16]!// Store x3 and x2 pair on the stack and decrement sp by 16 
    stp x1,x0,[sp,#-16]!// Store x1 and x0 pair on the stack and decrement sp by 16 
    mrs x0, elr_el1 // Move exception link register to x0
    mrs x1, spsr_el1 // Move saved program status register to x1
    stp x0,x1,[sp,#-16]!// Store x0 and x1 pair on the stack and decrement sp by 16
	mov x0, lr
	mov x1, sp
	stp x0,x1,[sp,#-16]!// Store x0 and x1 pair on the stack and decrement sp by 16

	mov x0, #12345 // Load a number to output to x0
	bl uart_putn // Branch with link to putn subroutine
	
	ldp x0,x1,[sp],#16 // Load x0 and x1 pair from the stack and increment sp by 16 
    mov lr, x0 // Move x0 to exception link register
    mov sp, x1 // Move x1 to saved program status register
	ldp x0,x1,[sp],#16 // Load x0 and x1 pair from the stack and increment sp by 16 
    msr elr_el1, x0 // Move x0 to exception link register
    msr spsr_el1, x1 // Move x1 to saved program status register
	ldp x1,x0,[sp],#16 // Load x1 and x0 pair from the stack and increment sp by 16 
    ldp x3,x2,[sp],#16 // Load x3 and x2 pair from the stack and increment sp by 16 
    ldp x5,x4,[sp],#16 // Load x5 and x4 pair from the stack and increment sp by 16 
    ldp x7,x6,[sp],#16 // Load x7 and x6 pair from the stack and increment sp by 16 
    ldp x9,x8,[sp],#16 // Load x9 and x8 pair from the stack and increment sp by 16 
    ldp x11,x10,[sp],#16 // Load x11 and
	ldp x13,x12,[sp],#16 // Load x11 and
	ldp x15,x14,[sp],#16 // Load x11 and
	ldp x17,x16,[sp],#16 // Load x11 and
    ldp x19,x18,[sp],#16 // Load x19 and x18 pair from the stack and increment sp by 16 
    ldp x21,x20,[sp],#16 // Load x21 and x20 pair from the stack and increment sp by 16 
    ldp x23,x22,[sp],#16 // Load x23 and x22 pair from the stack and increment sp by 16 
    ldp x25,x24,[sp],#16 // Load x25 and x24 pair from the stack and increment sp by 16 
    ldp x27,x26,[sp],#16 // Load x27 and x26 pair from the stack and increment sp by 16 
    ldp x29,x28,[sp],#16 // Load x29 and x28 pair from the stack and increment sp by 16 

	ret

