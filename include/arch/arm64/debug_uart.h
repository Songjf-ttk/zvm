/**
 * @file debug_uart.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-04-04
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef __DEBUG_UART_H__
#define __DEBUG_UART_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#ifdef CONFIG_ZVM_EARLYPRINT_MSG

int save_boot_params(void);

int tpl_main(void);

void printascii(const char *str);

void asm_print_debug(unsigned long long reg, unsigned long long reg2);

int debug_printf(const char *fmt, ...);

#endif 

#endif  /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif
