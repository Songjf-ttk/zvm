/*
 * Copyright (c) 2022 xcl (xiongcl@hnu.edu.cn)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VM_CPU_H_
#define ZEPHYR_INCLUDE_ZVM_VM_CPU_H_

#include <kernel.h>
#include <kernel/thread.h>
#include <kernel_structs.h>

#include <_zvm/zvm.h>

#ifdef CONFIG_PREEMPT_ENABLED
/* positive num */
#define VCPU_RT_PRIO        RT_VM_WORK_PRIORITY
#define VCPU_NORT_PRIO      NORT_VM_WORK_PRIORITY
#else
/* negetive num */
#define VCPU_RT_PRIO        K_HIGHEST_THREAD_PRIO + RT_VM_WORK_PRIORITY
#define VCPU_NORT_PRIO      K_HIGHEST_THREAD_PRIO + NORT_VM_WORK_PRIORITY
#endif

#define VCPU_IPI_MASK_ALL   (0xffffffff)

/* For clear warning for unknow reason */
struct vcpu;

/**
 * @brief allocate a vcpu struct and init it.
 */
struct vcpu *vm_vcpu_init(struct vm *vm, uint16_t vcpu_id, char *vcpu_name);

/**
 * @brief start the vcpu instance.
 */
int vm_vcpu_run(struct vcpu *vcpu);
int vm_vcpu_pause(struct vcpu *vcpu);
int vm_vcpu_halt(struct vcpu *vcpu);

/**
 * @brief vcpu run func entry.
 */
int z_vcpu_run(struct vcpu *vcpu);

int vcpu_irq_exit(struct vcpu *vcpu);
int vcpu_state_switch(struct k_thread *thread, uint16_t new_state);

void do_vcpu_swap(struct k_thread *new_thread, struct k_thread *old_thread);
void do_asm_vcpu_swap(struct k_thread *new_thread, struct k_thread *old_thread);

/**
 * @brief vcpu ipi schduler to inform system schduler to schdule vcpu.
 */
int vcpu_ipi_scheduler(uint32_t cpu_mask, uint32_t timeout);

#endif /* ZEPHYR_INCLUDE_ZVM_VM_CPU_H_ */
