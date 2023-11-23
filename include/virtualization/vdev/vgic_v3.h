/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_VGIC_V3_H_
#define ZEPHYR_INCLUDE_ZVM_ARM_VGIC_V3_H_

#include <zephyr.h>
#include <kernel.h>
#include <devicetree.h>
#include <spinlock.h>
#include <drivers/interrupt_controller/gic.h>
#include <arch/arm64/sys_io.h>
#include <virtualization/vm_dev.h>
#include <virtualization/vm_irq.h>
#include <virtualization/zvm.h>
#include <virtualization/vdev/vgic_common.h>

/* SGI mode */
#define SGI_SIG_TO_LIST		(0)
#define SGI_SIG_TO_OTHERS	(1)

/* vgic macro here */
#define VGIC_MAX_VCPU       64
#define VGIC_UNDEFINE_ADDR  0xFFFFFFFF

/* vgic action */
#define ACTION_CLEAR_VIRQ	    BIT(0)
#define ACTION_SET_VIRQ		    BIT(1)

/* GIC control value */
#define GICH_VMCR_VENG0			(1 << 0)
#define GICH_VMCR_VENG1			(1 << 1)
#define GICH_VMCR_VACKCTL		(1 << 2)
#define GICH_VMCR_VFIQEN		(1 << 3)
#define GICH_VMCR_VCBPR			(1 << 4)
#define GICH_VMCR_VEOIM			(1 << 9)
#define GICH_VMCR_DEFAULT_MASK  (0xf8 << 24)

#define GICH_HCR_EN       		(1 << 0)
#define GICH_HCR_UIE      		(1 << 1)
#define GICH_HCR_LRENPIE  		(1 << 2)
#define GICH_HCR_NPIE     		(1 << 3)

/* list register */
#define LIST_REG_GTOUP0			(0)
#define LIST_REG_GROUP1			(1)
#define LIST_REG_NHW_VIRQ		(0)
#define LIST_REG_HW_VIRQ		(1)

/* GICR registers offset from RDIST_base(n) */
#define VGICR_CTLR				0x0000
#define VGICR_IIDR				0x0004
#define VGICR_TYPER				0x0008
#define VGICR_STATUSR			0x0010
#define VGICR_WAKER				0x0014
#define VGICR_PROPBASER			0x0070
#define VGICR_PENDBASER			0x0078
#define VGICR_ISENABLER0		0x0100
#define VGICR_ICENABLER0		0x0180
#define VGICR_SGI_PENDING		0x0200
#define VGICR_PIDR2				0xFFE8

/* 在枚举中定义常量和标签的宏的名称都是大写的 */
/* System support list reg count */
enum {
	vgicv3_lr0,
	vgicv3_lr1,
	vgicv3_lr2,
	vgicv3_lr3,
	vgicv3_lr4,
	vgicv3_lr5,
	vgicv3_lr6,
	vgicv3_lr7,
	vgicv3_lrs_count,
};

/**
 * @brief vcpu vgicv3 register interface.
 */
struct gicv3_vcpuif_ctxt {
   	uint64_t ich_lr0_el2;
	uint64_t ich_lr1_el2;
	uint64_t ich_lr2_el2;
	uint64_t ich_lr3_el2;
	uint64_t ich_lr4_el2;
	uint64_t ich_lr5_el2;
	uint64_t ich_lr6_el2;
	uint64_t ich_lr7_el2;

	uint32_t ich_ap0r2_el2;
	uint32_t ich_ap1r2_el2;
	uint32_t ich_ap0r1_el2;
	uint32_t ich_ap1r1_el2;
	uint32_t ich_ap0r0_el2;
	uint32_t ich_ap1r0_el2;
	uint32_t ich_vmcr_el2;
	uint32_t ich_hcr_el2;

	uint32_t icc_ctlr_el1;
	uint32_t icc_sre_el1;
	uint32_t icc_pmr_el1;
};

/**
 * @brief gicv3_list_reg register bit field, which
 * provides interrupt context information for the virtual
 * CPU interface.
 */
struct gicv3_list_reg {
	uint64_t vINTID 	: 32;
	uint64_t pINTID 	: 13;
	uint64_t res0 		: 3;
	uint64_t priority 	: 8;
	uint64_t res1 		: 3;
	uint64_t nmi 		: 1;
	uint64_t group 		: 1;
	uint64_t hw 		: 1;
	uint64_t state 		: 2;
};

/**
 * @brief Virtual generatic interrupt controller redistributor
 * struct for each vm's vcpu.
 * Each Redistributor defines four 64KB frames as follows:
 * 1. RD_base
 * 2. SGI_base
 * 3. VLPI_base
 * 4. Reserved
 * @TODO: support vlpi later.
*/
struct virt_gic_gicr {
	uint32_t vcpu_id;

	/* virtual gicr for emulating device for vm. */
	uint32_t *gicr_rd_reg_base;
	uint32_t *gicr_sgi_reg_base;

	/* vm's physical gicr address. */
	uint32_t gicr_rd_base;
	uint32_t gicr_sgi_base;
	uint32_t gicr_rd_size;
	uint32_t gicr_sgi_size;

	struct k_spinlock gicr_lock;
};

/**
 * @brief vgicv3 virtual device struct, for emulate device.
 */
struct vgicv3_dev {
	struct virt_gic_gicd gicd;
	struct virt_gic_gicr *gicr[VGIC_RDIST_SIZE/VGIC_RD_SGI_SIZE];
};

/**
 * @brief virtual gicv3 device information.
*/
struct gicv3_vdevice {
	uint64_t gicd_base;
	uint64_t gicd_size;
	uint64_t gicr_base;
	uint64_t gicr_size;
};

/**
 * @brief gic vcpu interface init.
 */
int vcpu_gicv3_init(struct gicv3_vcpuif_ctxt *ctxt);


/**
 * @brief before enter vm, we need to load the vcpu interrupt state.
 */
int vgicv3_state_load(struct vcpu *vcpu, struct gicv3_vcpuif_ctxt *ctxt);

/**
 * @brief before exit from vm, we need to store the vcpu interrupt state.
 */
int vgicv3_state_save(struct vcpu *vcpu, struct gicv3_vcpuif_ctxt *ctxt);

/**
 * @brief send a virq to vm for el1 trap.
 */
int gicv3_inject_virq(struct vcpu *vcpu, struct virt_irq_desc *desc);

/**
 * @brief Get virq state from register.
 */
uint8_t gicv3_get_lr_state(struct vcpu *vcpu, struct virt_irq_desc *virq);

/**
 * @brief update virt irq flags aim to reset virq here.
 */
int gicv3_update_lr(struct vcpu *vcpu, struct virt_irq_desc *desc, int action);

/**
 * @brief gic redistribute vdev mem read.
 */
int vgic_gicrsgi_mem_read(struct vcpu *vcpu, struct virt_gic_gicr *gicr,
			uint32_t offset, uint64_t *v);

/**
 * @brief gic redistribute sgi vdev mem write
 */
int vgic_gicrsgi_mem_write(struct vcpu *vcpu, struct virt_gic_gicr *gicr,
			uint32_t offset, uint64_t *v);

/**
 * @brief gic redistribute rd vdev mem read
 */
int vgic_gicrrd_mem_read(struct vcpu *vcpu, struct virt_gic_gicr *gicr,
			uint32_t offset, uint64_t *v);

/**
 * @brief gic redistribute rd vdev mem write.
 */
int vgic_gicrrd_mem_write(struct vcpu *vcpu, struct virt_gic_gicr *gicr,
			uint32_t offset, uint64_t *v);

/**
 * @brief get gicr address type.
 */
int get_vcpu_gicr_type(struct virt_gic_gicr *gicr, uint32_t addr, uint32_t *offset);

/**
 * @brief raise a sgi signal to a vcpu.
 */
int vgicv3_raise_sgi(struct vcpu *vcpu, unsigned long sgi_value);

/**
 * @brief init vgicv3 device for the vm.
*/
struct vgicv3_dev *vgicv3_dev_init(struct vm *vm);

static ALWAYS_INLINE uint64_t gicv3_read_lr(uint8_t list_register)
{
	switch (list_register) {
	case 0:
		return read_sysreg(ICH_LR0_EL2);
	case 1:
		return read_sysreg(ICH_LR1_EL2);
	case 2:
		return read_sysreg(ICH_LR2_EL2);
	case 3:
		return read_sysreg(ICH_LR3_EL2);
	case 4:
		return read_sysreg(ICH_LR4_EL2);
	case 5:
		return read_sysreg(ICH_LR5_EL2);
	case 6:
		return read_sysreg(ICH_LR6_EL2);
	case 7:
		return read_sysreg(ICH_LR7_EL2);
	default:
		return 0;
	}
}

static ALWAYS_INLINE void gicv3_write_lr(uint8_t list_register, uint64_t value)
{
	switch (list_register) {
	case 0:
		write_sysreg(value, ICH_LR0_EL2);
		break;
	case 1:
		write_sysreg(value, ICH_LR1_EL2);
		break;
	case 2:
		write_sysreg(value, ICH_LR2_EL2);
		break;
	case 3:
		write_sysreg(value, ICH_LR3_EL2);
		break;
	case 4:
		write_sysreg(value, ICH_LR4_EL2);
		break;
	case 5:
		write_sysreg(value, ICH_LR5_EL2);
		break;
	case 6:
		write_sysreg(value, ICH_LR6_EL2);
		break;
	case 7:
		write_sysreg(value, ICH_LR7_EL2);
		break;
	default:
		return;
	}
}

#endif /* ZEPHYR_INCLUDE_ZVM_ARM_VGIC_V3_H_ */
