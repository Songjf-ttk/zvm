/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <device.h>
#include <drivers/pm_cpu_ops/psci.h>
#include <virtualization/zvm.h>
#include <virtualization/vm_device.h>
#include <virtualization/arm/mm.h>
#include <virtualization/arm/trap_handler.h>
#include <virtualization/arm/cpu.h>
#include <virtualization/arm/vtimer.h>
#include <virtualization/vdev/vgic_v3.h>
#include <virtualization/vdev/vgic_common.h>
#include <virtualization/vdev/vpsci.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

static uint64_t wzr_reg = 0;

uint64_t* find_index_reg(uint16_t index, arch_commom_regs_t *regs)
{
    uint64_t *value;

    if (index == 31) {
        value = NULL;
    } else if (index > 18 && index < 30) {
        value = &regs->callee_saved_regs.x19 + index - 19;
    } else {
        value = (index == 30) ? &regs->esf_handle_regs.x30 : (&regs->esf_handle_regs.x0 + index);
    }
    return value;
}

static int handle_ftrans_desc(int iss_dfsc, uint64_t pa_addr,
            struct esr_dabt_area *dabt, arch_commom_regs_t *regs)
{
    int ret = 0;
    struct vcpu *vcpu = _current_vcpu;
    uint64_t esr_elx = vcpu->arch->fault.esr_el2;

#ifdef CONFIG_VM_DYNAMIC_MEMORY
    /* TODO: Add dynamic memory allocate. */
#else
    uint16_t reg_index = dabt->srt;
    uint64_t *reg_value;
    reg_value = find_index_reg(reg_index, regs);
    if (reg_value == NULL) {
        reg_value = &wzr_reg;
    }

    /* check that if it is a device memory fault */
    ret = handle_vm_device_emulate(vcpu->vm, pa_addr);
    if(ret){
        /* pci initial sucessful. */
        if(ret > 0){
            return 0;
        }
        reg_value = find_index_reg(reg_index, regs);
        *reg_value = 0xfefefefefefefefe;
        ZVM_LOG_ERR("Unable to handle Date abort in address: 0x%llx ! \n", pa_addr);
        ZVM_LOG_ERR("A stage-2 translation table need to set for this device address 0x%llx.\n", pa_addr);
        /**
         * if the device is allocated, whether it can be emulated
         * by virtIO?
        */
    }else{
        ret = vm_mem_domain_partitions_add(vcpu->vm->vmem_domain);
        vcpu->arch->ctxt.regs.pc -= (GET_ESR_IL(esr_elx)) ? 4 : 2;
    }
#endif
    return ret;
}

static int handle_faccess_desc(int iss_dfsc, uint64_t pa_addr,
            struct esr_dabt_area *dabt, arch_commom_regs_t *regs)
{
    int ret;
    uint8_t size;
    uint16_t reg_index = dabt->srt;
    uint16_t iss_isv, iss_sas;
    uint64_t addr = pa_addr, *reg_value;

    iss_isv = dabt->isv;
    if (!iss_isv) {
        ZVM_LOG_WARN("Instruction syndrome not valid\n");
        return -EFAULT;
    }

    reg_value = find_index_reg(reg_index, regs);
    if (reg_value == NULL) {
        reg_value = &wzr_reg;
    }

    iss_sas = dabt->sas;
    switch (iss_sas) {
    case ISS_SAS_8BIT:
        size = 1;
        break;
    case ISS_SAS_16BIT:
        size = 2;
        break;
    case ISS_SAS_32BIT:
        size = 4;
        break;
    case ISS_SAS_64BIT:
        size = 8;
        break;
    default:
        ZVM_LOG_WARN("unsupport data size\n");
        return -EFAULT;
    }

    ret = vdev_mmio_abort(regs, dabt->wnr, addr, reg_value, size);
    if (ret < 0) {
        ZVM_LOG_WARN("Handle mmio read/write failed! The addr: %llx \n", addr);
        return -ENODEV;
    }
    return ret;
}

static int cpu_unknwn_sync(arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    ARG_UNUSED(arch_ctxt);
    ARG_UNUSED(esr_elx);
    ZVM_LOG_WARN("Unknow sync type! \n ");
	return 0;
}

static int cpu_wfi_wfe_sync(arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    uint32_t condition, esr_iss;
    struct vcpu *vcpu = _current_vcpu;

    esr_iss = GET_ESR_ISS(esr_elx);
    if(esr_iss & BIT(ESR_ISS_CV_SHIFT)){
        condition = GET_ESR_ISS_COND(esr_elx);
        if((condition & 0x1) && (condition != 0xf)){
            return -ESRCH;
        }
    }else{
        /* TODO: support aarch32 VM.*/
        return -ESRCH;
    }
    /* WFE */
    if(esr_iss & 0x01){
        if(vcpu->vcpu_state == _VCPU_STATE_RUNNING){
            vm_vcpu_ready(vcpu);
        }
    }else{  /* WFI */
        vcpu_wait_for_irq(vcpu);
    }

	return 0;
}

static int cpu_dmcr_mrc_sync(arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    ARG_UNUSED(arch_ctxt);
    ARG_UNUSED(esr_elx);
	return 0;
}

static int cpu_dmcrr_mrrc_sync(arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    ARG_UNUSED(arch_ctxt);
    ARG_UNUSED(esr_elx);
	return 0;
}

static int cpu_simd_fp_sync(arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    ARG_UNUSED(arch_ctxt);
    ARG_UNUSED(esr_elx);
	return 0;
}

static int cpu_il_exe_sync(arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    ARG_UNUSED(arch_ctxt);
    ARG_UNUSED(esr_elx);
	return 0;
}


#define BIT_MASK0(last, first) \
	((0xffffffffffffffffULL >> (64 - ((last) + 1 - (first)))) << (first))
#define GET_FIELD(value, last, first) \
	(((value) & BIT_MASK0((last), (first))) >> (first))

static int cpu_hvc64_sync(struct vcpu *vcpu, arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    int ret = 0;
    unsigned long hvc_imm;

#ifdef CONFIG_ZVM_TIME_MEASURE
    vm_irq_timing_print();
#endif
    hvc_imm = GET_FIELD((esr_elx), 15, 0);
    /*hvc_imm != 0 means that it is not a psci hvc.*/
    if(hvc_imm) {
        ret = zvm_service_vmops(hvc_imm);
        return ret;
    }

    ret = do_psci_call(vcpu, arch_ctxt);

	return ret;
}

static int cpu_system_msr_mrs_sync(arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    uint32_t reg_index, reg_name;
    uint32_t this_esr = esr_elx;
    uint64_t *reg_value ;
    struct esr_sysreg_area *esr_sysreg = (struct esr_sysreg_area *)&this_esr;
    struct vcpu *vcpu = _current_vcpu;

    reg_index = esr_sysreg->rt;
    /* the operation is write */
    if (!esr_sysreg->dire) {
        reg_value = find_index_reg(reg_index, arch_ctxt);
    }

    reg_name = this_esr & ESR_SYSINS_REGS_MASK;
    switch (reg_name) {
    /* supporte sgi related register here */
    case ESR_SYSINSREG_SGI0R_EL1:
    case ESR_SYSINSREG_SGI1R_EL1:
	case ESR_SYSINSREG_ASGI1R_EL1:
		if (!esr_sysreg->dire) {
            vgicv3_raise_sgi(vcpu, *reg_value);
        }
		break;
	case ESR_SYSINSREG_CNTPCT_EL0:
    /* The process for VM's timer, emulate timer register access */
	case ESR_SYSINSREG_CNTP_TVAL_EL0:
        simulate_timer_cntp_tval(vcpu, esr_sysreg->dire, reg_value);
		break;
	case ESR_SYSINSREG_CNTP_CTL_EL0:
        simulate_timer_cntp_ctl(vcpu, esr_sysreg->dire, reg_value);
		break;
	case ESR_SYSINSREG_CNTP_CVAL_EL0:
		simulate_timer_cntp_cval(vcpu, esr_sysreg->dire, reg_value);
		break;

	default:
        ZVM_LOG_WARN("Can not emulate provided register here, the register is 0x%x \n", reg_name);
        return -ENOVDEV;
		break;
	}

	return 0;
}

static int cpu_inst_abort_low_sync(arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    uint64_t ipa_ddr;
    ipa_ddr = get_fault_ipa(read_hpfar_el2(), read_far_el2());
    ARG_UNUSED(arch_ctxt);
    ARG_UNUSED(esr_elx);
	return 0;
}

static int cpu_inst_abort_cur_sync(arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    ARG_UNUSED(arch_ctxt);
    ARG_UNUSED(esr_elx);
	return 0;
}

static int cpu_misaligned_pc_sync(arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    ARG_UNUSED(arch_ctxt);
    ARG_UNUSED(esr_elx);
	return 0;
}

static int cpu_data_abort_low_sync(arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    int ret, iss_dfsc;
    uint64_t ipa_ddr;
    uint64_t db_esr = esr_elx;
    struct esr_dabt_area *dabt = (struct esr_dabt_area *)&db_esr;

    iss_dfsc = dabt->dfsc & ~(0x3);
    ipa_ddr = get_fault_ipa(read_hpfar_el2(), read_far_el2());

    switch (iss_dfsc) {
    /* translation fault level0-3*/
    case DFSC_FT_TRANS_L3:
    case DFSC_FT_TRANS_L2:
    case DFSC_FT_TRANS_L1:
    case DFSC_FT_TRANS_L0:
        ret = handle_ftrans_desc(iss_dfsc, ipa_ddr, dabt, arch_ctxt);
        break;
    /* access fault level0-3*/
    case DFSC_FT_ACCESS_L3:
    case DFSC_FT_ACCESS_L2:
    case DFSC_FT_ACCESS_L1:
    case DFSC_FT_ACCESS_L0:
        ret = handle_faccess_desc(iss_dfsc, ipa_ddr, dabt, arch_ctxt);
        break;
    /* premission fault level0-3*/
    case DFSC_FT_PERM_L3:
    case DFSC_FT_PERM_L2:
    case DFSC_FT_PERM_L1:
    case DFSC_FT_PERM_L0:
    default:
        ZVM_LOG_WARN("Stage-2 error without translation fault: %016llx !  VM stop! \n", ipa_ddr);
        ret = -ENOVDEV;
        break;
    }

    return ret;
}

static int cpu_data_abort_cur_sync(arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    ARG_UNUSED(arch_ctxt);
    ARG_UNUSED(esr_elx);
	return 0;
}

static int cpu_misaligned_sp_sync(arch_commom_regs_t *arch_ctxt, uint64_t esr_elx)
{
    ARG_UNUSED(arch_ctxt);
    ARG_UNUSED(esr_elx);
	return 0;
}


int arch_vm_trap_sync(struct vcpu *vcpu)
{
    int err = 0;
    uint64_t esr_elx;
    arch_commom_regs_t *arch_ctxt;

    esr_elx = vcpu->arch->fault.esr_el2;
    arch_ctxt = &vcpu->arch->ctxt.regs;
    switch (GET_ESR_EC(esr_elx)) {
        case 0b000000: /* 0x00: "Unknown reason" */
            err = cpu_unknwn_sync(arch_ctxt, esr_elx);
            break;
        case 0b000001: /* 0x01: "Trapped WFI or WFE instruction execution" */
            err = cpu_wfi_wfe_sync(arch_ctxt, esr_elx);
            break;
        case 0b000011: /* 0x03: "Trapped MCR or MRC access  */
            err = cpu_dmcr_mrc_sync(arch_ctxt, esr_elx);
            break;
        case 0b000100: /* 0x04: "Trapped MCRR or MRRC access */
            err = cpu_dmcrr_mrrc_sync(arch_ctxt, esr_elx);
            break;
        case 0b000101: /* 0x05 */
        case 0b000110: /* 0x06 */
            goto handler_failed;
            break;
        case 0b000111: /* 0x07: "Trapped access to SVE, Advanced SIMD, or
                floating-point functionality" */
            err = cpu_simd_fp_sync(arch_ctxt, esr_elx);
            break;
        case 0b001100: /* 0x0c */
        case 0b001101: /* 0x0d */
            goto handler_failed;
            break;
        case 0b001110: /* 0x0e: "Illegal Execution state" */
            err = cpu_il_exe_sync(arch_ctxt, esr_elx);
            break;
        case 0b010001: /* 0x11 */
            goto handler_failed;
            break;
        case 0b010110: /* 0x16: "HVC instruction execution in AArch64 state" */
            err = cpu_hvc64_sync(vcpu, arch_ctxt, esr_elx);
            break;
        case 0b011000: /* 0x18: "Trapped MSR, MRS or System instruction execution in
                AArch64 state */
            err = cpu_system_msr_mrs_sync(arch_ctxt, esr_elx);
            break;
        case 0b011001: /* 0x19 */
            goto handler_failed;
            break;
        case 0b100000: /* 0x20: "Instruction Abort from a lower Exception level, that
                might be using AArch32 or AArch64" */
            err = cpu_inst_abort_low_sync(arch_ctxt, esr_elx);
            break;
        case 0b100001: /* 0x21: "Instruction Abort taken without a change in Exception level." */
            err = cpu_inst_abort_cur_sync(arch_ctxt, esr_elx);
            break;
        case 0b100010: /* 0x22: "PC alignment fault exception." */
            err = cpu_misaligned_pc_sync(arch_ctxt, esr_elx);
            break;
        case 0b100100: /* 0x24: "Data Abort from a lower Exception level, that might
                be using AArch32 or AArch64" */
            err = cpu_data_abort_low_sync(arch_ctxt, esr_elx);
            break;
        case 0b100101: /* 0x25: "Data Abort taken without a change in Exception level" */
            err = cpu_data_abort_cur_sync(arch_ctxt, esr_elx);
            break;
        case 0b100110: /* 0x26: "SP alignment fault exception" */
            err = cpu_misaligned_sp_sync(arch_ctxt, esr_elx);
            break;
        case 0b101000: /* 0x28 */
        case 0b101100: /* 0x2c */
        case 0b101111: /* 0x2f */
        case 0b110000: /* 0x30 */
        default:
            goto handler_failed;
	}

    if (GET_ESR_EC(esr_elx) != 0b010110)
        vcpu->arch->ctxt.regs.pc += (GET_ESR_IL(esr_elx)) ? 4 : 2;
    return err;

handler_failed:
    ZVM_LOG_WARN("ZVM do not support this exit code: %lld. \n", GET_ESR_EC(esr_elx));
    return -ENOVDEV;
}

void* z_vm_lower_sync_handler(uint64_t esr_elx)
{
    struct vcpu *vcpu = _current_vcpu;
    if (vcpu == NULL) {
        ZVM_LOG_WARN("Get vcpu struct failed ");
    }

	vcpu->arch->fault.esr_el2 = esr_elx;
    return (void*)vcpu;
}

void* z_vm_lower_irq_handler(z_arch_esf_t *esf_ctxt)
{
    ARG_UNUSED(esf_ctxt);
    struct vcpu *vcpu = _current_vcpu;
    if (vcpu == NULL) {
        ZVM_LOG_WARN("Get vcpu struct failed ");
    }

    return (void *)vcpu;
}
