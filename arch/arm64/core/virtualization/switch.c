/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>
#include <kernel/sched_priq.h>
#include <arch/arm64/lib_helpers.h>
#include <arch/cpu.h>

#include <virtualization/arm/cpu.h>
#include <virtualization/arm/asm.h>
#include <virtualization/zvm.h>
#include <virtualization/arm/sysreg.h>
#include <virtualization/arm/switch.h>
#include <virtualization/arm/trap_handler.h>
#include <virtualization/vdev/vgic_common.h>

#include <drivers/interrupt_controller/gic.h>
#include <arch/arm64/cpu.h>
#include <stdint.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

/* VM entry point */
extern int guest_vm_entry(struct vcpu *vcpu,struct zvm_vcpu_context *context);

static void vm_disable_daif(void)
{
    disable_debug_exceptions();
    disable_serror_exceptions();
    disable_fiq();

    disable_irq();
    isb();
}

static void vm_enable_daif(void)
{
    enable_debug_exceptions();
    enable_fiq();
    enable_serror_exceptions();

    enable_irq();
    isb();
}

static int vm_flush_vgic(struct vcpu *vcpu)
{
    int ret = 0;
    ret = virt_irq_flush_vgic(vcpu);
    if (ret) {
        ZVM_LOG_ERR("Flush vgic info failed, Unknow reason \n");
    }
    return ret;
}

static int vm_sync_vgic(struct vcpu *vcpu)
{
    int ret = 0;

    ret = virt_irq_sync_vgic(vcpu);
    if (ret) {
        ZVM_LOG_ERR("Sync vgic info failed, Unknow reason \n");
    }
    return ret;
}

static int arch_vm_irq_trap(struct vcpu *vcpu)
{
    ARG_UNUSED(vcpu);
    vm_enable_daif();
    return 0;
}

static void arch_vm_serror_trap(struct vcpu *vcpu, int exit_code)
{
    uint64_t disr;
    uint64_t esr;

    if (ARM_VM_SERROR_PENDING(exit_code)) {
        disr = vcpu->arch->fault.disr_el1;

        esr = (0x2f << 26);
        if(disr & BIT(24))
            esr |= (disr & ((1<<25) - 1));
        else
            esr |= (disr & (0x7<<10 | 0x1<<9 | 0x3f));
    }
}

void get_zvm_host_context(void)
{
    uint64_t hostctxt_addr ;
    struct k_thread *thread = _current;
    struct vcpu *vcpu = thread->vcpu_struct;

    hostctxt_addr = (uint64_t)&(vcpu->arch->host_ctxt);
    __asm__ volatile(
        "mov x11, %0"
        : : "r" (hostctxt_addr) :
    );
}

int arch_vcpu_run(struct vcpu *vcpu)
{
    int ret;
    uint16_t exit_type = 0;

    /* mask all interrupt here to disable interrupt */
    vm_disable_daif();
    ret = vm_flush_vgic(vcpu);
    if (ret) {
        return ret;
    }
    switch_to_guest_sysreg(vcpu);
    ZVM_LOG_INFO("The entry pc is: %08lx \n ", vcpu->arch->ctxt.regs.pc);

    /* Jump to the fire too! */
    exit_type = guest_vm_entry(vcpu, &vcpu->arch->host_ctxt);
    vcpu->exit_type = exit_type;

    ZVM_LOG_INFO("exit_type: %d, esr_el2: %08lx \n ", exit_type, read_esr_el2());
 //   ZVM_LOG_INFO("esr_el1: %08lx, far_el1: %08lx \n ", read_esr_el12(), read_far_el12());
    switch_to_host_sysreg(vcpu);

    vm_sync_vgic(vcpu);
    switch (exit_type) {
	case ARM_VM_EXCEPTION_SYNC:
        ret = arch_vm_trap_sync(vcpu);
        break;
    case ARM_VM_EXCEPTION_IRQ:
        ret = arch_vm_irq_trap(vcpu);
        break;
	case ARM_VM_EXCEPTION_SERROR:
        arch_vm_serror_trap(vcpu, exit_type);
        ZVM_LOG_WARN("SError exception type in this stage....\n");
        break;
    case ARM_VM_EXCEPTION_IRQ_IN_SYNC:
        ret = arch_vm_irq_trap(vcpu);
        break;
	default:
		ZVM_LOG_WARN("Unsupported exception type in this stage....\n");
        ZVM_LOG_WARN("Exit code: 0x%08llx \t exit_type: 0x%08x  ....\n", read_esr_el2(), exit_type);
		return -ESRCH;
        break;
	}

    if (vcpu->vm->vm_status == VM_STATE_HALT) {
        ret = -ESRCH;
    }
    return ret;
}

void z_vm_switch_handle_pre(uint32_t irq)
{
    bool *bit_addr;
    struct k_thread *thread;
    struct vcpu *vcpu;

    if( (vcpu = _current_vcpu) == NULL){
        return;
    }

    bit_addr = vcpu->vm->vm_irq_block.irq_bitmap;
    /* If it is a vcpu thread, judge whether the signal is send to it */
    if(!bit_addr[irq]){
        return;
    }

    thread = vcpu->work->vcpu_thread;
    thread->base.thread_state |= _THREAD_VCPU_NO_SWITCH;
}
