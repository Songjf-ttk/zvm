/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_SWITCH_H__
#define ZEPHYR_INCLUDE_ZVM_ARM_SWITCH_H__

#include <kernel.h>
#include <zephyr.h>
#include <stdint.h>

#include <_zvm/arm/cpu.h>
#include <_zvm/vm.h>


#define GET_HOST_LR ({	      uint64_t val;				\
	__asm__ volatile ("mov %0 , lr"	:"=r" (val) : : );	\
    val;                                 })


#define SET_HOST_LR(val)  ({									\
	__asm__ volatile ("mov x30, %0"	 :: "r" (val) : "memory");	\
})

/**
 * @brief Get the zvm host context object for context switch
 */
void get_zvm_host_context(void);

/**
 * @brief ready to run vcpu here, for prepare running guest code.
 * This function aim to make preparetion before running guest os and restore
 * the origin hardware state after guest exit.
 */
int arch_vcpu_run(struct vcpu *vcpu);

/**
 * @brief Avoid switch handle when current thread is a vcpu thread.
 */
void z_vm_switch_handle_pre(uint32_t irq);


#endif /* ZEPHYR_INCLUDE_ZVM_ARM_SWITCH_H__ */
