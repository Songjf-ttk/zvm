/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DEBUG_UART_H__
#define __DEBUG_UART_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

int save_boot_params(void);

int tpl_main(void);

void printascii(const char *str);

void early_print_debug(unsigned long long reg, unsigned long long reg2);
void asm_register_print_smp(unsigned long long reg0, unsigned long long reg1);

void show_print_debug_delay(void);

int debug_earlyprint(const char *fmt, ...);

#endif  /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif
