/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_TIMER_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_TIMER_H_

#ifndef _ASMLANGUAGE

#include <drivers/timer/arm_arch_timer.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONFIG_ZVM

#define ARM_ARCH_TIMER_IRQ	ARM_TIMER_VIRTUAL_IRQ
#define ARM_ARCH_TIMER_PRIO	ARM_TIMER_VIRTUAL_PRIO
#define ARM_ARCH_TIMER_FLAGS	ARM_TIMER_VIRTUAL_FLAGS

#else

#ifdef	CONFIG_HAS_ARM_VHE_EXTN
#define ARM_ARCH_TIMER_IRQ	ARM_TIMER_HYP_IRQ
#define ARM_ARCH_TIMER_PRIO	ARM_TIMER_HYP_PRIO
#define ARM_ARCH_TIMER_FLAGS	ARM_TIMER_HYP_FLAGS
#else	// !CONFIG_HAS_ARM_VHE_EXTN
#define ARM_ARCH_TIMER_IRQ	ARM_TIMER_NON_SECURE_IRQ
#define ARM_ARCH_TIMER_PRIO	ARM_TIMER_NON_SECURE_PRIO
#define ARM_ARCH_TIMER_FLAGS	ARM_TIMER_NON_SECURE_FLAGS
#endif

#endif // !CONFIG_ZVM


static ALWAYS_INLINE void arm_arch_timer_init(void)
{
}


#ifndef CONFIG_ZVM

static ALWAYS_INLINE void arm_arch_timer_set_compare(uint64_t val)
{
	write_cntv_cval_el0(val);
}

static ALWAYS_INLINE void arm_arch_timer_enable(unsigned char enable)
{
	uint64_t cntv_ctl;

	cntv_ctl = read_cntv_ctl_el0();

	if (enable) {
		cntv_ctl |= CNTV_CTL_ENABLE_BIT;
	} else {
		cntv_ctl &= ~CNTV_CTL_ENABLE_BIT;
	}

	write_cntv_ctl_el0(cntv_ctl);
}

static ALWAYS_INLINE void arm_arch_timer_set_irq_mask(bool mask)
{
	uint64_t cntv_ctl;

	cntv_ctl = read_cntv_ctl_el0();

	if (mask) {
		cntv_ctl |= CNTV_CTL_IMASK_BIT;
	} else {
		cntv_ctl &= ~CNTV_CTL_IMASK_BIT;
	}

	write_cntv_ctl_el0(cntv_ctl);
}

static ALWAYS_INLINE uint64_t arm_arch_timer_count(void)
{
	return read_cntvct_el0();
}

#else

static ALWAYS_INLINE void arm_arch_timer_set_compare(uint64_t val)
{
	write_cntp_cval_el0(val);
}

static ALWAYS_INLINE void arm_arch_timer_enable(unsigned char enable)
{
	uint64_t cntp_ctl;

	cntp_ctl = read_cntp_ctl_el0();

	if (enable) {
		cntp_ctl |= CNTP_CTL_ENABLE_BIT;
	} else {
		cntp_ctl &= ~CNTP_CTL_ENABLE_BIT;
	}

	write_cntp_ctl_el0(cntp_ctl);
}

static ALWAYS_INLINE void arm_arch_timer_set_irq_mask(bool mask)
{
	uint64_t cntp_ctl;

	cntp_ctl = read_cntp_ctl_el0();

	if (mask) {
		cntp_ctl |= CNTP_CTL_IMASK_BIT;
	} else {
		cntp_ctl &= ~CNTP_CTL_IMASK_BIT;
	}

	write_cntp_ctl_el0(cntp_ctl);
}

static ALWAYS_INLINE uint64_t arm_arch_timer_count(void)
{
	return read_cntpct_el0();
}
#endif // !CONFIG_ZVM

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_TIMER_H_ */
