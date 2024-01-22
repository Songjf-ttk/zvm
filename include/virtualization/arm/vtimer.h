/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_VTIMER_H_
#define ZEPHYR_INCLUDE_ZVM_ARM_VTIMER_H_

#include <kernel.h>
#include <zephyr.h>
#include <sys/time_units.h>

#include <virtualization/zvm.h>
#include <virtualization/vm_irq.h>

#define ARM_ARCH_VIRT_VTIMER_IRQ	ARM_TIMER_VIRTUAL_IRQ
#define ARM_ARCH_VIRT_VTIMER_PRIO	ARM_TIMER_VIRTUAL_PRIO
#define ARM_ARCH_VIRT_VTIMER_FLAGS	ARM_TIMER_VIRTUAL_FLAGS

#define ARM_ARCH_VIRT_PTIMER_IRQ	ARM_TIMER_NON_SECURE_IRQ
#define ARM_ARCH_VIRT_PTIMER_PRIO	ARM_TIMER_NON_SECURE_PRIO
#define ARM_ARCH_VIRT_PTIMER_FLAGS	ARM_TIMER_NON_SECURE_FLAGS

#define HOST_CYC_PER_TICK	((uint64_t)sys_clock_hw_cycles_per_sec() \
			/ (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)

typedef void (*z_timer_func_t)(uint16_t, struct virt_irq_desc *, void *);

/**
 * @brief Virtual timer context for this vcpu.
 * Describes only two elements, one is a virtual timer, the other is physical.
 */
struct virt_timer_context {
	/* virtual timer irq number */
	uint32_t virt_virq;
	uint32_t virt_pirq;
	/* control register */
	uint32_t cntv_ctl;
	uint32_t cntp_ctl;
	/* virtual count compare register */
	uint64_t cntv_cval;
	uint64_t cntp_cval;
	/* virtual count value register */
	uint64_t cntv_tval;
	uint64_t cntp_tval;
/* timeout for softirq */
#ifdef CONFIG_SYS_CLOCK_EXISTS
	/* this is vcpu entry in a timeout queue */
	struct _timeout vtimer_timeout;
	struct _timeout ptimer_timeout;
#endif /* CONFIG_SYS_CLOCK_EXISTS */
	/* vcpu timer offset value, value is cycle */
	uint64_t timer_offset;
	void *vcpu;
	bool enable_flag;
};

/**
 * @brief Architecture-related timer information,
 * including irq numbers for virtual and physical timers.
 */
struct zvm_arch_timer_info {
	uint32_t virt_irq;
	uint32_t phys_irq;
};

/**
 * @brief Set virtual timer compare value.
 *
 * @param val
 */
static ALWAYS_INLINE void arm_arch_virt_timer_set_compare(uint64_t val)
{
	write_cntv_cval_el02(val);
}

/**
 * @brief Enable timer here.
 *
 * @param enable
 */
static ALWAYS_INLINE void arm_arch_virt_timer_enable(bool enable)
{
	uint64_t cntv_ctl;

	cntv_ctl = read_cntv_ctl_el02();

	if (enable) {
		cntv_ctl |= CNTV_CTL_ENABLE_BIT;
	} else {
		cntv_ctl &= ~CNTV_CTL_ENABLE_BIT;
	}

	write_cntv_ctl_el02(cntv_ctl);
}

/**
 * @brief Set irq mask for virt timer.
 *
 * @param mask
 */
static ALWAYS_INLINE void arm_arch_virt_timer_set_irq_mask(bool mask)
{
	uint64_t cntv_ctl;

	cntv_ctl = read_cntv_ctl_el02();

	if (mask) {
		cntv_ctl |= CNTV_CTL_IMASK_BIT;
	} else {
		cntv_ctl &= ~CNTV_CTL_IMASK_BIT;
	}

	write_cntv_ctl_el02(cntv_ctl);
}


/**
 * @brief Get virtual timer irq number
 */
int zvm_arch_vtimer_init(void);

/**
 * @brief Simulate cntp_tval_el0 register
 */
void simulate_timer_cntp_tval(struct vcpu *vcpu, int read, uint64_t *value);

/**
 * @brief Simulate cntp_cval_el0 register
 */
void simulate_timer_cntp_cval(struct vcpu *vcpu, int read, uint64_t *value);

/**
 * @brief Simulate cntp_ctl register
 */
void simulate_timer_cntp_ctl(struct vcpu *vcpu, int read, uint64_t *value);

/**
 * @brief Init vtimer struct
 */
int arch_vcpu_timer_init(struct vcpu *vcpu);

#endif /* ZEPHYR_INCLUDE_ZVM_ARM_VTIMER_H_ */
