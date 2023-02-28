/*
 * Copyright (c) 2022 xcl (xiongcl@hnu.edu.cn)
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VM_CONSOLE_H_
#define ZEPHYR_INCLUDE_ZVM_VM_CONSOLE_H_

#include <zephyr.h>
#include <kernel.h>
#include <devicetree.h>

#include <_zvm/vdev/uart.h>
#include <_zvm/vm_dev.h>
#include <_zvm/vm_irq.h>
#include <_zvm/zvm.h>

/* VM debug console uart hardware info. */
#define VM_DEBUG_CONSOLE_BASE DT_REG_ADDR(DT_CHOSEN(vm_shell_uart))
#define VM_DEBUG_CONSOLE_SIZE DT_REG_SIZE(DT_CHOSEN(vm_shell_uart))

#define VM_DEBUG_CONSOLE_IRQ    DT_IRQ_BY_IDX(DT_CHOSEN(vm_shell_uart), 0, irq)
#define VM_DEFAULT_CONSOLE_IRQ  DEFAULT_VM_UART_IRQ
#define VM_CONSOLE_IRQ_PRIO     (0x25)

/**
 * @brief Uart handler function.
 */
void vm_uart_callback(const struct device *dev, void *user_data);

int vm_debug_console_add(struct vm *vm);
int vm_debug_console_remove(struct vm *vm, struct virt_dev *r_dev);

static ALWAYS_INLINE uint64_t get_vm_console_base(void)
{
    return DEFAULT_VM_UART_BASE;
}

static ALWAYS_INLINE uint64_t get_vm_console_size(void)
{
    return DEFAULT_VM_UART_SIZE;
}

#endif /* ZEPHYR_INCLUDE_ZVM_VM_CONSOLE_H_ */
