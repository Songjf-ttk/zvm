/*
 * Copyright 2021-2022 HNU-ESNL
 * Copyright 2023 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>
#include <arch/cpu.h>
#include <arch/arm64/lib_helpers.h>
#include <virtualization/arm/cpu.h>
#include <virtualization/vm.h>
#include <virtualization/vdev/vgic_v3.h>
#include <virtualization/arm/switch.h>
#include <virtualization/arm/sysreg.h>

void vcpu_sysreg_load(struct vcpu *vcpu)
{
    struct zvm_vcpu_context *g_context = &vcpu->arch->ctxt;

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
    g_context->regs.esf_handle_regs.elr = read_elr_el12();
    g_context->regs.esf_handle_regs.spsr = read_spsr_el12();
    vcpu->arch->vcpu_sys_register_loaded = false;
}


void switch_to_guest_sysreg(struct vcpu *vcpu)
{
    uint32_t reg_val;
    struct zvm_vcpu_context *gcontext = &vcpu->arch->ctxt;
    struct zvm_vcpu_context *hcontext = &vcpu->arch->host_ctxt;

    /* save host context */
    hcontext->running_vcpu = vcpu;
    hcontext->sys_regs[VCPU_SPSR_EL1] = read_spsr_el1();
    hcontext->sys_regs[VCPU_MDSCR_EL1] = read_mdscr_el1();

    /* load stage-2 pgd for vm */
    write_vtcr_el2(vcpu->vm->arch->vtcr_el2);
    write_vttbr_el2(vcpu->vm->arch->vttbr);

    /* enable hyperviosr trap */
    write_hcr_el2(vcpu->arch->hcr_el2);
    reg_val = read_cpacr_el1();
    reg_val &= ~CPACR_EL1_ZEN;
    reg_val = (CPACR_EL1_TTA | CPTR_EL2_TAM | CPACR_EL1_FPEN_NOTRAP);
    write_cpacr_el1(reg_val);
    write_vbar_el2((uint64_t)_hyp_vector_table);

    hcontext->sys_regs[VCPU_TPIDRRO_EL0] = read_tpidrro_el0();
    write_tpidrro_el0(gcontext->sys_regs[VCPU_TPIDRRO_EL0]);

    write_elr_el2(gcontext->regs.pc);
    write_spsr_el2(gcontext->regs.pstate);

    reg_val = ((struct gicv3_vcpuif_ctxt *)vcpu->arch->virq_data)->icc_ctlr_el1;
    reg_val &= ~(0x02);
    write_sysreg(reg_val, ICC_CTLR_EL1);

    write_mdscr_el1(gcontext->sys_regs[VCPU_MDSCR_EL1]);
}

void switch_to_host_sysreg(struct vcpu *vcpu)
{
    uint32_t reg_val;
    struct zvm_vcpu_context *gcontext = &vcpu->arch->ctxt;
    struct zvm_vcpu_context *hcontext = &vcpu->arch->host_ctxt;

    gcontext->sys_regs[VCPU_TPIDRRO_EL0] = read_tpidrro_el0();
    write_tpidrro_el0(hcontext->sys_regs[VCPU_TPIDRRO_EL0]);

    gcontext->regs.pc = read_elr_el2();
    gcontext->regs.pstate = read_spsr_el2();

    reg_val = ((struct gicv3_vcpuif_ctxt *)vcpu->arch->virq_data)->icc_ctlr_el1;
    reg_val |= (0x02);
    write_sysreg(reg_val, ICC_CTLR_EL1);

    gcontext->sys_regs[VCPU_MDSCR_EL1] = read_mdscr_el1();

    /* disable hyperviosr trap */
    if (vcpu->arch->hcr_el2 & HCR_VSE_BIT) {
        vcpu->arch->hcr_el2 = read_hcr_el2();
    }
    write_hcr_el2(HCR_VHE_FLAGS);
    write_vbar_el2((uint64_t)_vector_table);

    /* save vm's stage-2 pgd */
    vcpu->vm->arch->vtcr_el2 = read_vtcr_el2();
    vcpu->vm->arch->vttbr = read_vttbr_el2();

    /* load host context */
    write_mdscr_el1(hcontext->sys_regs[VCPU_MDSCR_EL1]);
    write_spsr_el1(hcontext->sys_regs[VCPU_SPSR_EL1]);
}
