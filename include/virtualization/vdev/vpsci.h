/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZVM_VPSCI_H
#define __ZVM_VPSCI_H

#include <virtualization/arm/cpu.h>

/* psci func for vcpu */
uint64_t psci_vcpu_suspend(struct vcpu *vcpu, arch_commom_regs_t *arch_ctxt);
uint64_t psci_vcpu_off(struct vcpu *vcpu, arch_commom_regs_t *arch_ctxt);
uint64_t psci_vcpu_affinity_info(struct vcpu *vcpu, arch_commom_regs_t *arch_ctxt);
uint64_t psci_vcpu_migration(struct vcpu *vcpu, arch_commom_regs_t *arch_ctxt);
uint64_t psci_vcpu_migration_info_type(struct vcpu *vcpu, arch_commom_regs_t *arch_ctxt);
uint64_t psci_vcpu_other(unsigned long psci_func);
uint64_t psci_vcpu_on(struct vcpu *vcpu, arch_commom_regs_t *arch_ctxt);

int do_psci_call(struct vcpu *vcpu, arch_commom_regs_t *arch_ctxt);

#endif