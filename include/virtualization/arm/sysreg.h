/*
 * Copyright 2021-2022 HNU
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
 * @brief Load guest system register.
*/
void switch_to_guest_sysreg(struct vcpu *vcpu);

/**
 * @brief switch_to_host_sysreg aim to save guest sysreg before exit.
 */
void switch_to_host_sysreg(struct vcpu *vcpu);

#endif /* ZEPHYR_INCLUDE_ZVM_ARM_SYSREG_H__ */
