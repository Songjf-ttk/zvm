/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VM_CONSOLE_H_
#define ZEPHYR_INCLUDE_ZVM_VM_CONSOLE_H_

#include <zephyr.h>
#include <kernel.h>
#include <devicetree.h>
#include <virtualization/vm_device.h>
#include <virtualization/vm_irq.h>
#include <virtualization/zvm.h>

/* VM debug console uart hardware info. */
#define VM_DEBUG_CONSOLE_BASE   DT_REG_ADDR(DT_CHOSEN(vm_console))
#define VM_DEBUG_CONSOLE_SIZE   DT_REG_SIZE(DT_CHOSEN(vm_console))
#define VM_DEBUG_CONSOLE_IRQ    DT_IRQN(DT_CHOSEN(vm_console))

#define VM_DEFAULT_CONSOLE_NAME     "UART"
#define VM_DEFAULT_CONSOLE_NAME_LEN (4)

int vm_console_create(struct vm *vm);

#endif /* ZEPHYR_INCLUDE_ZVM_VM_CONSOLE_H_ */
