/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_VGIC_COMMON_H_
#define ZEPHYR_INCLUDE_ZVM_ARM_VGIC_COMMON_H_

#include <zephyr.h>
#include <kernel.h>
#include <devicetree.h>

struct virt_dev;
struct vm_vgic_block;

#define VGIC_CONTROL_BLOCK_ID		vgic_control_block
#define VGIC_CONTROL_BLOCK_NAME		vm_irq_control_block

/* GIC version */
#define VGIC_V2     BIT(0)
#define VGIC_V3     BIT(8)

/* GIC dev type */
#define TYPE_GIC_GICD		BIT(0)
#define TYPE_GIC_GICR_RD	BIT(1)
#define TYPE_GIC_GICR_SGI	BIT(2)
#define TYPE_GIC_GICR_VLPI	BIT(3)
#define TYPE_GIC_INVAILD	(0xFF)

/* GIC device macro here */
#define VGIC_DIST_BASE	DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 0)
#define VGIC_DIST_SIZE	DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic), 0)
#define VGIC_RDIST_BASE	DT_REG_ADDR_BY_IDX(DT_INST(0, arm_gic), 1)
#define VGIC_RDIST_SIZE	DT_REG_SIZE_BY_IDX(DT_INST(0, arm_gic), 1)

/* Vgic control block flag */
#define VIRQ_HW_SUPPORT		BIT(1)

#define VGIC_DATA_IBP(data)		((struct vm_vgic_block *)data)->irq_bitmap
#define VGIC_DATA_VID(data) 	((struct vm_vgic_block *)data)->vm_virt_irq_desc
#define VGIC_DATA_VGD(data) 	((struct vm_vgic_block *)data)->vgic_data

#define VGIC_VIRQ_IN_SGI		(0x0)
#define VGIC_VIRQ_IN_PPI		(0x1)
#define VGIC_VIRQ_LEVEL_SORT(irq)	((irq)/VM_LOCAL_VIRQ_NR)


struct virt_gic_gicd {
	uint32_t gicd_ctlr;
	uint32_t gicd_typer;
	uint32_t gicd_pidr2;
	uint32_t addr_base;
	uint32_t addr_size;
	struct k_spinlock gicd_lock;
};

struct virt_gic_gicr {
	uint32_t gicr_ctlr;

	/* Provides information about the implementer and revision of the Redistributor */
	uint32_t gicr_iidr;
	uint32_t gicr_pidr2;

	uint32_t gicr_ispender;
	uint32_t gicr_enabler0;

	uint32_t raddr_base;
	uint32_t vcpu_id;

	uint32_t sgi_base;
	uint32_t vlpi_base;

	/* These must be 64 bits for record */
	uint64_t gicr_typer;

	sys_dlist_t list;

	struct k_spinlock gicr_lock;
};


typedef int (*vgic_gicd_read_32_t)(const struct device *dev, struct vcpu *vcpu,
					uint32_t offset, uint32_t *value);

typedef int (*vgic_gicrrd_read_32_t)(const struct device *dev, struct vcpu *vcpu,
					uint32_t offset, uint32_t *value);

typedef int (*vgic_gicd_write_32_t)(const struct device *dev, struct vcpu *vcpu,
					uint32_t offset, uint32_t *value);

typedef int (*vgic_gicrrd_write_32_t)(const struct device *dev, struct vcpu *vcpu,
					uint32_t offset, uint32_t *value);

__subsystem struct vgic_common_api {

	vgic_gicd_read_32_t 	vgicd_read_32;
	vgic_gicd_write_32_t	vgicd_write_32;

	vgic_gicrrd_read_32_t	vgicr_rd_read_32;
	vgic_gicrrd_write_32_t	vgicr_write_32;

};


typedef int (*vm_irq_exit_t)(struct device *dev, struct vcpu *vcpu, void *data);

typedef int (*vm_irq_enter_t)(struct device *dev, struct vcpu *vcpu, void *data);


__subsystem struct vm_irq_handler_api {
    vm_irq_exit_t irq_exit_from_vm;
    vm_irq_enter_t irq_enter_to_vm;
};

/**
 * @brief vcpu vgic control block.
 */
struct vcpu_vgic_block {

};

struct vm_vgic_block {

	bool enabled;
	bool irq_bitmap[VM_GLOBAL_VIRQ_NR];

	uint32_t irq_num;
	uint32_t cpu_num;

	/* vgic control block flag */
	uint32_t flags;

	uint32_t irq_target[VM_GLOBAL_VIRQ_NR];
	uint32_t sgi_vcpu_source[CONFIG_MP_NUM_CPUS][VM_SGI_VIRQ_NR];

	/* virq description */
    struct virt_irq_desc *vm_virt_irq_desc;

	/* Bind to vgic device */
	void *vgic_data;

};

void z_ready_thread(struct k_thread *thread);

/**
 * @brief test and set vgic bits.
 */
void vgic_irq_test_and_set_bit(struct vcpu *vcpu, uint32_t spi_nr_count, uint32_t *value,
						uint32_t bit_size, int enable);

/**
 * @brief Just for enable menopoly irq for vcpu.
 */
void arch_vdev_irq_enable(struct vcpu *vcpu);

/**
 * @brief Just for disable menopoly irq for vcpu.
 */
void arch_vdev_irq_disable(struct vcpu *vcpu);


int vgic_vdev_mem_read(struct virt_dev *vdev, arch_commom_regs_t *regs, 
                            uint64_t addr, uint64_t *value);

int vgic_vdev_mem_write(struct virt_dev *vdev, arch_commom_regs_t *regs, 
                            uint64_t addr, uint64_t *value);

/**
 * @brief send a virt irq signal to a vcpu.
 */
int set_virq_to_vcpu(struct vcpu *vcpu, uint32_t virq_num);

/**
 * @brief send a virq to vm that bind to this vcpu.
 */
int set_virq_to_vm(struct vm *vm, uint32_t virq_num);


int virt_irq_sync_vgic(struct vcpu *vcpu);

int virt_irq_flush_vgic(struct vcpu *vcpu);

/**
 * @brief Get the virq desc object.
 */
struct virt_irq_desc *get_virt_irq_desc(struct vcpu *vcpu, uint32_t virq);

/**
 * @brief common call for creating control block.
 */
int vm_irq_ctrlblock_create(struct device *unused, struct vm *vm);

/**
 * @brief Create and init vm virq object for each vm.
 */
int vm_virq_desc_init(struct vm *vm, void *args);

/**
 * @brief vgic init common function.
 */
int zvm_arch_vgic_init(void *op);

/**
 * @brief When vcpu is loop on idel mode, we must send virq to it
 * for activating it.
 */
static ALWAYS_INLINE void wakeup_target_vcpu(struct vcpu *vcpu, struct virt_irq_desc *desc)
{
    ARG_UNUSED(desc);
    /* Set thread into runnig queue */
    z_ready_thread(vcpu->work->vcpu_thread);
}

/**
 * @brief Check that whether this vm can recieve virq?
 */
static ALWAYS_INLINE bool is_vm_irq_valid(struct vm *vm, uint32_t flag)
{
    if (vm->vm_status == VM_STATE_NEVER_RUN) {
		return false;
	}

    if (vm->vm_status == VM_STATE_PAUSE) {
        if (flag & VIRT_IRQ_IS_WAKEUP) {
			return true;
		} else {
			return false;
		}
    }
    return true;
}

#endif /* ZEPHYR_INCLUDE_ZVM_ARM_VGIC_COMMON_H_ */
