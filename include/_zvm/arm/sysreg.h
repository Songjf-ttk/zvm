/*
 * Copyright (c) 2022 xcl (xiongcl@hnu.edu.cn)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_SYSREG_H__
#define ZEPHYR_INCLUDE_ZVM_ARM_SYSREG_H__

#include <zephyr.h>
#include <stdint.h>

/**
 * @brief vcpu_sysreg_loads_vhe - Load guest system registers to the physical CPU.
 */
void vcpu_sysreg_load(struct vcpu *vcpu);

/**
 * @brief store system register to vcpu struct for keeping the VM state.
 */
void vcpu_sysreg_save(struct vcpu *vcpu);

/**
 * @brief vm_sysreg_load aim to load guest sysreg.
*/
void vm_sysreg_load(struct zvm_vcpu_context *context);

/**
 * @brief vm_sysreg_save aim to save guest sysreg before exit.
 */
void vm_sysreg_save(struct zvm_vcpu_context *context);

#endif /* ZEPHYR_INCLUDE_ZVM_ARM_SYSREG_H__ */
