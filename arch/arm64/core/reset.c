/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel_internal.h>
#include <arch/arm64/lib_helpers.h>
#include "boot.h"
#include <virtualization/arm/cpu.h>

uint64_t cpu_vmpidr_el2_list[CONFIG_MP_NUM_CPUS] = {0};

void __weak z_arm64_el_highest_plat_init(void)
{
	/* do nothing */
}

void __weak z_arm64_el3_plat_init(void)
{
	/* do nothing */
}

void __weak z_arm64_el2_plat_init(void)
{
	/* do nothing */
}

void __weak z_arm64_el1_plat_init(void)
{
	/* do nothing */
}

void z_arm64_el_highest_init(void)
{
	if (is_el_highest_implemented()) {
		write_cntfrq_el0(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
	}

	z_arm64_el_highest_plat_init();

	isb();
}

enum el3_next_el {
	EL3_TO_EL2,
	EL3_TO_EL1_NO_EL2,
	EL3_TO_EL1_SKIP_EL2
};

static inline enum el3_next_el el3_get_next_el(void)
{
	if (!is_el_implemented(2)) {
		return EL3_TO_EL1_NO_EL2;
	} else if (is_in_secure_state() && !is_el2_sec_supported()) {
		/*
		 * Is considered an illegal return "[..] a return to EL2 when EL3 is
		 * implemented and the value of the SCR_EL3.NS bit is 0 if
		 * ARMv8.4-SecEL2 is not implemented" (D1.11.2 from ARM DDI 0487E.a)
		 */
		return EL3_TO_EL1_SKIP_EL2;
	} else {
		return EL3_TO_EL2;
	}
}

/* announce here */
void z_arm64_el2_init(void);
void z_arm64_el2_vhe_init(void);

void z_arm64_el3_init(void)
{
	uint64_t reg;

	/* Setup vector table */
	write_vbar_el3((uint64_t)_vector_table);
	isb();

	reg = 0U;			/* Mostly RES0 */
	reg &= ~(CPTR_TTA_BIT |		/* Do not trap sysreg accesses */
		 CPTR_TFP_BIT |		/* Do not trap SVE, SIMD and FP */
		 CPTR_TCPAC_BIT);	/* Do not trap CPTR_EL2 / CPACR_EL1 accesses */
	write_cptr_el3(reg);

	reg = 0U;			/* Reset */
#ifdef CONFIG_ARMV8_A_NS
	reg |= SCR_NS_BIT;		/* EL2 / EL3 non-secure */
#endif
	reg |= (SCR_RES1 |		/* RES1 */
		SCR_RW_BIT |		/* EL2 execution state is AArch64 */
		SCR_ST_BIT |		/* Do not trap EL1 accesses to timer */
		SCR_HCE_BIT |		/* Do not trap HVC */
		SCR_SMD_BIT);		/* Do not trap SMC */
	write_scr_el3(reg);

#if defined(CONFIG_GIC_V3)
	reg = read_sysreg(ICC_SRE_EL3);
	reg = (ICC_SRE_ELx_SRE_BIT |	/* System register interface is used */
	       ICC_SRE_EL3_EN_BIT);	/* Enables lower Exception level access to ICC_SRE_EL1 */
	write_sysreg(reg, ICC_SRE_EL3);
#endif

	z_arm64_el3_plat_init();

	isb();

	if (el3_get_next_el() == EL3_TO_EL1_SKIP_EL2) {
		/*
		 * handle EL2 init in EL3, as it still needs to be done,
		 * but we are going to be skipping EL2.
		 */
#if	defined(CONFIG_ZVM) && defined(CONFIG_HAS_ARM_VHE_EXTN)
		/*TODO: May be delete here. */
		z_arm64_el2_vhe_init();
#else
		z_arm64_el2_init();
#endif
	}
}

void z_arm64_el2_init(void)
{
	uint64_t reg;

	reg = read_sctlr_el2();
	reg |= (SCTLR_EL2_RES1 |	/* RES1 */
		SCTLR_I_BIT |		/* Enable i-cache */
		SCTLR_SA_BIT);		/* Enable SP alignment check */
	write_sctlr_el2(reg);

	reg = read_hcr_el2();
	/* when EL2 is enable in current security status:
	 * Clear TGE bit: All exceptions that would not be routed to EL2;
	 * Clear AMO bit: Physical SError interrupts are not taken to EL2 and EL3.
	 * Clear IMO bit: Physical IRQ interrupts are not taken to EL2 and EL3.
	 */
	reg &= ~(HCR_IMO_BIT | HCR_AMO_BIT | HCR_TGE_BIT);
	reg |= HCR_RW_BIT;		/* EL1 Execution state is AArch64 */
	write_hcr_el2(reg);

	reg = 0U;			/* RES0 */
	reg |= CPTR_EL2_RES1;		/* RES1 */
	reg &= ~(CPTR_TFP_BIT |		/* Do not trap SVE, SIMD and FP */
		 CPTR_TCPAC_BIT);	/* Do not trap CPACR_EL1 accesses */
	write_cptr_el2(reg);

	zero_cntvoff_el2();		/* Set 64-bit virtual timer offset to 0 */
	zero_cnthctl_el2();
#ifdef CONFIG_CPU_AARCH64_CORTEX_R
	zero_cnthps_ctl_el2();
#else
	zero_cnthp_ctl_el2();
#endif
	/*
	 * Enable this if/when we use the hypervisor timer.
	 * write_cnthp_cval_el2(~(uint64_t)0);
	 */

	z_arm64_el2_plat_init();
	isb();
}

#if defined(CONFIG_ZVM) && defined(CONFIG_HAS_ARM_VHE_EXTN)
/* Configure EL2/virtualization related registers.
 * TODO: system register info must be standarded later.
 */
void z_arm64_el2_vhe_init(void)
{
	bool vhe_flag = false;
	uint64_t reg;			/* 64bit register */

	/* Set EL2 mmu off */
	reg = SCTLR_EL2_RES1;
	write_sctlr_el2(reg);
	isb();

	/* hvc trap to hypervisor*/
	reg = 1UL << 31;
	write_hcr_el2(reg);
	isb();

	reg = read_id_aa64mmfr1_el1();
	reg = ASM_UBFX(8, 4, reg);

	if(reg){
		vhe_flag = true;
	}

	if(vhe_flag){
		reg = HCR_VHE_FLAGS;
	}else{
		reg = HCR_NVHE_FLAGS;
	}
	//debug_print_pre(reg, 0xabcd);
	write_hcr_el2(reg);

	if(vhe_flag){
		zero_sysreg(cntvoff_el2);
	}else{
		/* Enable EL1 physical timer and clear vitrtual offset */
		reg = 0x03;
		write_cnthctl_el2(reg);
		zero_sysreg(cntvoff_el2);
	}

	/* gicv3 supporte */
#if defined(CONFIG_GIC_V3)
	reg = read_id_aa64pfr0_el1();
	reg = ASM_UBFX(24, 4, reg);
	if(reg){
		__asm__ volatile(
			"mrs x0, s3_4_c12_c9_5 \n"
			"orr x0, x0, #0x01 \n"
			"orr x0, x0, #0x08 \n"
			"msr s3_4_c12_c9_5, x0 \n"
			"isb \n"
			"mrs x0, s3_4_c12_c9_5 \n"
			"tbz x0, #0, 99f \n"
			"msr s3_4_c12_c11_0, xzr \n "
			"99: 	\n"
		);
	}
	/* enable EOI mode, for passthrough device.  */
	reg = read_sysreg(ICC_CTLR_EL1);
	reg |= 0x02;
	write_sysreg(reg, ICC_CTLR_EL1);

#endif

	/* Get identification information for the PE from midr_el1,
		Set it to vpidr_el2 to info virtualization process id*/
	reg = read_midr_el1();
	write_vpidr_el2(reg);

	reg = read_mpidr_el1();
	write_vmpidr_el2(reg);

	/*Get mpidr info for VM.*/
	cpu_vmpidr_el2_list[MPIDR_TO_CORE(GET_MPIDR())] = reg;

	/* Controls trapping to EL2 of accesses to CPACR, CPACR_EL1, trace */
	reg = 0x33ff;
	write_cptr_el2(reg);		// Disable copro. traps to EL2

	/* Disable CP15 trapping to EL2 of EL1 accesses to the System register  */
	zero_sysreg(hstr_el2);

	/* Debug related init  */
	zero_sysreg(mdcr_el2);

	/* Stage-2 translation base register init*/
	zero_sysreg(vttbr_el2);

	/* Set Exception type to EL1h, spsr_el2 hold the process state when exception happen */
	reg = INIT_PSTATE_EL1 | SPSR_DAIF_MASK;
	write_spsr_el2(reg);
	isb();

	z_arm64_el2_plat_init();
}
#endif  /* CONFIG_ZVM */

void z_arm64_el1_init(void)
{
	uint64_t reg;

	/* Setup vector table */
	write_vbar_el1((uint64_t)_vector_table);
	isb();

	reg = 0U;			/* RES0 */
	reg |= CPACR_EL1_FPEN_NOTRAP;	/* Do not trap NEON/SIMD/FP initially */
					/* TODO: CONFIG_FLOAT_*_FORBIDDEN */
	write_cpacr_el1(reg);

	reg = read_sctlr_el1();
	reg |= (SCTLR_EL1_RES1 |	/* RES1 */
		SCTLR_I_BIT |		/* Enable i-cache */
		SCTLR_SA_BIT);		/* Enable SP alignment check */
	write_sctlr_el1(reg);

	write_cntv_cval_el0(~(uint64_t)0);
#if defined(CONFIG_ZVM) && defined(CONFIG_HAS_ARM_VHE_EXTN)
	write_cntp_cval_el0(~(uint64_t)0);	/* Can not access cntp_el0 in el1 */
#endif
	/*
	 * Enable these if/when we use the corresponding timers.
	 * write_cntp_cval_el0(~(uint64_t)0);
	 * write_cntps_cval_el1(~(uint64_t)0);
	 */

	z_arm64_el1_plat_init();

	isb();
}

void z_arm64_el3_get_next_el(uint64_t switch_addr){
	uint64_t spsr;

	write_elr_el3(switch_addr);

	/* Mask the DAIF */
	spsr = SPSR_DAIF_MASK;

	if (el3_get_next_el() == EL3_TO_EL2) {
		/* Dropping into EL2 */
		spsr |= SPSR_MODE_EL2T;
	} else {
		/* Dropping into EL1 */
		spsr |= SPSR_MODE_EL1T;
	}

	write_spsr_el3(spsr);
}

/*
 * operation for all data cache
 * ops:  K_CACHE_INVD: invalidate
 *	 K_CACHE_WB: clean
 *	 K_CACHE_WB_INVD: clean and invalidate
 */
void arch_flush_dcache_all(void)
{
	uint8_t loc, ctype, cache_level, line_size, way_pos;
	uint32_t clidr_el1, csselr_el1, ccsidr_el1;
	uint32_t max_ways, max_sets, dc_val, set, way;

	/* Data barrier before start */
	dsb();
	clidr_el1 = read_clidr_el1();

	loc = (clidr_el1 >> 24) & BIT_MASK(3);
	if (!loc)
		return;

	for (cache_level = 0; cache_level < loc; cache_level++) {
		ctype = (clidr_el1 >> (cache_level*3))
				& BIT_MASK(3);
		/* No data cache, continue */
		if (ctype < 2)
			continue;

		/* select cache level */
		csselr_el1 = cache_level << 1;
		write_csselr_el1(csselr_el1);
		isb();

		ccsidr_el1 = read_ccsidr_el1();
		line_size = (ccsidr_el1 >> 0
				& BIT_MASK(3)) + 4;
		max_ways = (ccsidr_el1 >> 3)
				& BIT_MASK(10);
		max_sets = (ccsidr_el1 >> 13)
				& BIT_MASK(15);
		/* 32-log2(ways), bit position of way in DC operand */
		way_pos = __builtin_clz(max_ways);

		for (set = 0; set <= max_sets; set++) {
			for (way = 0; way <= max_ways; way++) {
				/* way number, aligned to pos in DC operand */
				dc_val = way << way_pos;
				/* cache level, aligned to pos in DC operand */
				dc_val |= csselr_el1;
				/* set number, aligned to pos in DC operand */
				dc_val |= set << line_size;
				__asm__ volatile ("dc cisw, %0" :: "r" (dc_val) : "memory");

			}
		}
	}
	/* Restore csselr_el1 to level 0 */
	write_csselr_el1(0);
	dsb();
	isb();
	return;
}
