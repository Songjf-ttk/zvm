/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>
#include <arch/cpu.h>
#include <arch/arm64/lib_helpers.h>
#include <_zvm/arm/cpu.h>
#include <_zvm/vm.h>
#include <_zvm/arm/vgic_v3.h>
#include <_zvm/arm/switch.h>
#include <_zvm/arm/sysreg.h>

void vcpu_sysreg_load(struct vcpu *vcpu)
{
    struct zvm_vcpu_context *g_context = &vcpu->arch->ctxt;
    struct zvm_vcpu_context *h_context = &vcpu->arch->host_ctxt;
    ARG_UNUSED(h_context);

    write_csselr_el1(g_context->sys_regs[VCPU_CSSELR_EL1]);
    write_vmpidr_el2(g_context->sys_regs[VCPU_MPIDR_EL1]);
    write_sctlr_el12(g_context->sys_regs[VCPU_SCTLR_EL1]);
    write_tcr_el12(g_context->sys_regs[VCPU_TCR_EL1]);
    write_cpacr_el12(g_context->sys_regs[VCPU_CPACR_EL1]);
    write_ttbr0_el12(g_context->sys_regs[VCPU_TTBR0_EL1]);
    write_ttbr1_el12(g_context->sys_regs[VCPU_TTBR1_EL1]);
    write_esr_el12(g_context->sys_regs[VCPU_ESR_EL1]);
    write_afsr0_el12(g_context->sys_regs[VCPU_AFSR0_EL1]);
    write_afsr1_el12(g_context->sys_regs[VCPU_AFSR1_EL1]);
    write_far_el12(g_context->sys_regs[VCPU_FAR_EL1]);
    write_mair_el12(g_context->sys_regs[VCPU_MAIR_EL1]);
    write_vbar_el12(g_context->sys_regs[VCPU_VBAR_EL1]);
    write_contextidr_el12(g_context->sys_regs[VCPU_CONTEXTIDR_EL1]);
    write_amair_el12(g_context->sys_regs[VCPU_AMAIR_EL1]);
    write_cntkctl_el12(g_context->sys_regs[VCPU_CNTKCTL_EL1]);
    write_par_el1(g_context->sys_regs[VCPU_PAR_EL1]);
    write_tpidr_el1(g_context->sys_regs[VCPU_TPIDR_EL1]);
    write_sp_el1(g_context->sys_regs[VCPU_SP_EL1]);
    write_elr_el12(g_context->sys_regs[VCPU_ELR_EL1]);
    write_spsr_el12(g_context->sys_regs[VCPU_SPSR_EL1]);
    vcpu->arch->vcpu_sys_register_loaded = true;
    write_hstr_el2(BIT(15));
    vcpu->arch->host_mdcr_el2 = read_mdcr_el2();
    write_mdcr_el2(vcpu->arch->guest_mdcr_el2);
}

void vcpu_sysreg_save(struct vcpu *vcpu)
{
    struct zvm_vcpu_context *g_context = &vcpu->arch->ctxt;
    struct zvm_vcpu_context *h_context = &vcpu->arch->host_ctxt;
    ARG_UNUSED(h_context);

    g_context->sys_regs[VCPU_MPIDR_EL1] = read_vmpidr_el2();
    g_context->sys_regs[VCPU_CSSELR_EL1] = read_csselr_el1();
    g_context->sys_regs[VCPU_ACTLR_EL1] = read_actlr_el1();

    g_context->sys_regs[VCPU_SCTLR_EL1] = read_sctlr_el12();
    g_context->sys_regs[VCPU_CPACR_EL1] = read_cpacr_el12();
    g_context->sys_regs[VCPU_TTBR0_EL1] = read_ttbr0_el12();
    g_context->sys_regs[VCPU_TTBR1_EL1] = read_ttbr1_el12();
    g_context->sys_regs[VCPU_ESR_EL1] = read_esr_el12();
    g_context->sys_regs[VCPU_TCR_EL1] = read_tcr_el12();
    g_context->sys_regs[VCPU_AFSR0_EL1] = read_afsr0_el12();
    g_context->sys_regs[VCPU_AFSR1_EL1] = read_afsr1_el12();
    g_context->sys_regs[VCPU_FAR_EL1] = read_far_el12();
    g_context->sys_regs[VCPU_MAIR_EL1] = read_mair_el12();
    g_context->sys_regs[VCPU_VBAR_EL1] = read_vbar_el12();
    g_context->sys_regs[VCPU_CONTEXTIDR_EL1] = read_contextidr_el12();
    g_context->sys_regs[VCPU_AMAIR_EL1] = read_amair_el12();
    g_context->sys_regs[VCPU_CNTKCTL_EL1] = read_cntkctl_el12();

    g_context->sys_regs[VCPU_PAR_EL1] = read_par_el1();
    g_context->sys_regs[VCPU_TPIDR_EL1] = read_tpidr_el1();

    g_context->regs.callee_saved_regs.sp_elx = read_sp_el1();
    g_context->regs.esf_handle_regs.elr = read_elr_el12();
    g_context->regs.esf_handle_regs.spsr = read_spsr_el12();

    vcpu->arch->vcpu_sys_register_loaded = false;
}

void vm_sysreg_load(struct zvm_vcpu_context *context)
{
    uint32_t reg_val;
    struct vcpu *vcpu = _current_vcpu;

    write_sp_el0(context->regs.callee_saved_regs.sp_el0);
    write_elr_el2(context->regs.pc);
    write_spsr_el2(context->regs.pstate);

    reg_val = vcpu->arch->vgicv3_context->icc_ctlr_el1;
    reg_val &= ~(0x02);
    write_sysreg(reg_val, ICC_CTLR_EL1);
}

void vm_sysreg_save(struct zvm_vcpu_context *context)
{
    uint32_t reg_val;
    struct vcpu *vcpu = _current_vcpu;

    context->regs.pc = read_elr_el2();
    context->regs.pstate = read_spsr_el2();

    reg_val = vcpu->arch->vgicv3_context->icc_ctlr_el1;
    reg_val |= (0x02);
    write_sysreg(reg_val, ICC_CTLR_EL1);

    context->sys_regs[VCPU_MDSCR_EL1] = read_mdscr_el1();
    context->regs.callee_saved_regs.sp_el0 = read_sp_el0();
}
