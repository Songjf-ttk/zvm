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
#include <virtualization/arm/cpu.h>
#include <virtualization/vm.h>

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
 * @brief Avoid switch handle when current thread is a vcpu thread,
 * and curretn irq is send to vcpu.
 * @retval
 * true: this irq is sent to vcpu.
 * false: this irq is a normal irq.
 */
bool zvm_switch_handle_pre(uint32_t irq);


#endif /* ZEPHYR_INCLUDE_ZVM_ARM_SWITCH_H__ */
