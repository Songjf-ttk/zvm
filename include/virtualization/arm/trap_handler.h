/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_TRAP_HANDLER_H_
#define ZEPHYR_INCLUDE_ZVM_ARM_TRAP_HANDLER_H_

#include <kernel.h>
#include <zephyr.h>
#include <arch/arm64/exc.h>


#define VM_SGI_VIRQ_NR		(16)
#define VM_PPI_VIRQ_NR		(16)
#define VM_SPI_VIRQ_NR 		(224)

#define VM_LOCAL_VIRQ_NR	(VM_SGI_VIRQ_NR + VM_PPI_VIRQ_NR)
#define VM_GLOBAL_VIRQ_NR   (VM_LOCAL_VIRQ_NR + VM_SPI_VIRQ_NR)


#define   AARCH64_INST_ADJUST    (0x04)


/* HPFAR_EL2 addr mask */
#define HPFAR_EL2_MASK			GENMASK(39,4)
#define HPFAR_EL2_SHIFT			(4)
#define HPFAR_EL2_PAGE_MASK   	GENMASK(12,0)
#define HPFAR_EL2_PAGE_SHIFT	(12)

struct vcpu;

struct esr_dabt_area {
	uint64_t dfsc   :6;     /* Data Fault Status Code */
	uint64_t wnr    :1;     /* Write / not Read */
	uint64_t s1ptw  :1;     /* Stage 2 fault during stage 1 translation */
	uint64_t cm     :1;     /* Cache Maintenance */
	uint64_t ea     :1;     /* External Abort Type */
	uint64_t fnv    :1;     /* FAR not Valid */
    uint64_t set    :2;     /* Synchronous Error Type */
	uint64_t vncr   :1;     /* Indicates that the fault came from use of VNCR_EL2.*/
	uint64_t ar     :1;     /* Acquire Release */
	uint64_t sf     :1;     /* Sixty Four bit register */
	uint64_t srt    :5;     /* The Register which store the value */
	uint64_t sse    :1;     /* Sign extend */
	uint64_t sas    :2;     /* Syndrome Access Size */
	uint64_t isv    :1;     /* Syndrome Valid */
	uint64_t il     :1;     /* Instruction length */
	uint64_t ec     :6;     /* Exception Class */
    uint64_t ISS2   :5;     /* FEAT_LS64 is Implemented */
	uint64_t res	:27;	/* RES0 */
};

struct esr_sysreg_area {
	uint64_t dire	:1;   /* Direction */
	uint64_t crm	:4;    /* CRm */
	uint64_t rt		:5;    /* Rt */
	uint64_t crn	:4;    /* CRn */
	uint64_t op1	:3;    /* Op1 */
	uint64_t op2	:3;    /* Op2 */
	uint64_t op0	:2;    /* Op0 */
	uint64_t res0	:3;		/* reserved file */
	uint64_t il		:1;    /* Instruction length */
	uint64_t ec		:6;	   /* Exception Class */
};

int arch_vm_trap_sync(struct vcpu *vcpu);

/**
 * @brief sync handler for this vm.
 */
void* z_vm_lower_sync_handler(uint64_t esr_elx);

/**
 * @brief irq handler for this vm.
 */
void* z_vm_lower_irq_handler(z_arch_esf_t *esf_ctxt);


static ALWAYS_INLINE uint64_t get_fault_ipa(uint64_t hpfar_ipa, uint64_t far_ipa)
{
    uint64_t fault_ipa;
    fault_ipa = hpfar_ipa & HPFAR_EL2_MASK;
    fault_ipa = (fault_ipa >> HPFAR_EL2_SHIFT) << HPFAR_EL2_PAGE_SHIFT;
    fault_ipa |= far_ipa & HPFAR_EL2_PAGE_MASK;

    return fault_ipa;
}

#endif	/*ZEPHYR_INCLUDE_ZVM_ARM_TRAP_HANDLER_H_*/
