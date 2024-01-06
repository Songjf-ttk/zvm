/*
 * Copyright 2021-2022 HNU-ESNL
 * Copyright 2023 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <zephyr.h>
#include <device.h>
#include <kernel_structs.h>
#include <arch/cpu.h>
#include <arch/arm64/lib_helpers.h>
#include <arch/common/sys_bitops.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>
#include <drivers/interrupt_controller/gic.h>
#include <logging/log.h>
#include <../drivers/interrupt_controller/intc_gicv3_priv.h>
#include <virtualization/arm/cpu.h>
#include <virtualization/arm/asm.h>
#include <virtualization/vdev/vgic_v3.h>
#include <virtualization/vdev/vgic_common.h>
#include <virtualization/zvm.h>
#include <virtualization/vm_irq.h>
#include <virtualization/vm_console.h>
#include <virtualization/vm_dev.h>
#include <virtualization/vdev/virt_device.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

static struct virt_irq_desc *vgic_get_virt_irq_desc(struct vcpu *vcpu, uint32_t virq)
{
	struct vm *vm = vcpu->vm;

    /* sgi virq num */
	if (virq < VM_LOCAL_VIRQ_NR) {
        return &vcpu->virq_block.vcpu_virt_irq_desc[virq];
    }

    /* spi virq num */
    if((virq >= VM_LOCAL_VIRQ_NR) && (virq < VM_GLOBAL_VIRQ_NR)) {
		return &vm->vm_irq_block.vm_virt_irq_desc[virq - VM_LOCAL_VIRQ_NR];
    }

	return NULL;
}

static int vgic_irq_enable(struct vcpu *vcpu, uint32_t virt_irq)
{
	struct virt_irq_desc *desc;

	desc = vgic_get_virt_irq_desc(vcpu, virt_irq);
	if (!desc) {
        return -ENOENT;
    }

    desc->virq_flags |= VIRQ_ENABLED_FLAG;
	if (virt_irq > VM_LOCAL_VIRQ_NR) {
		if (desc->virq_flags & VIRQ_HW_FLAG) {
            if (desc->pirq_num > VM_LOCAL_VIRQ_NR)
                irq_enable(desc->pirq_num);
            else {
                ZVM_LOG_WARN("Not a spi interrupt!");
                return -ENODEV;
            }
        }
	} else {
        irq_enable(virt_irq);
    }

	return 0;
}

static int vgic_irq_disable(struct vcpu *vcpu, uint32_t virt_irq)
{
	struct virt_irq_desc *desc;

	desc = vgic_get_virt_irq_desc(vcpu, virt_irq);
	if (!desc) {
		return -ENOENT;
	}

	desc->virq_flags &= ~VIRQ_ENABLED_FLAG;
	if (virt_irq > VM_LOCAL_VIRQ_NR) {
		if (desc->virq_flags & VIRQ_HW_FLAG) {
            if (desc->pirq_num > VM_LOCAL_VIRQ_NR) {
				irq_disable(desc->pirq_num);
			} else {
                ZVM_LOG_WARN("Not a spi interrupt!");
                return -ENODEV;
            }
        }
	} else {
        irq_disable(virt_irq);
    }
	return 0;
}

static int virt_irq_set_type(struct vcpu *vcpu, uint32_t virt_irq, int value)
{
	struct virt_irq_desc *desc;

	desc = vgic_get_virt_irq_desc(vcpu, virt_irq);
	if (!desc) {
		return -ENOENT;
	}

	if (desc->type != value) {
		desc->type = value;
		if (desc->virq_flags & VIRQ_HW_FLAG) {
			if (value) {
				value = IRQ_TYPE_EDGE;
			} else {
				value = IRQ_TYPE_LEVEL;
			}
		}
	}
	return 0;
}

/**
 * @brief Set priority for specific virtual interrupt requests
 */
static int vgic_virq_set_priority(struct vcpu *vcpu, uint32_t virt_irq, int prio)
{
	struct virt_irq_desc *desc;

	desc = vgic_get_virt_irq_desc(vcpu, virt_irq);
	if (!desc) {
		return -ENOENT;
	}
	desc->prio = prio;

	return 0;
}

static int vgic_set_virq(struct vcpu *vcpu, struct virt_irq_desc *desc)
{
	uint8_t lr_state;
    k_spinlock_key_t key;
    struct vcpu_virt_irq_block *vb = &vcpu->virq_block;

    if (!is_vm_irq_valid(vcpu->vm, desc->virq_flags)) {
        ZVM_LOG_WARN("VM can not recieve virq signal, \
						VM's name: %s.", vcpu->vm->vm_name);
        return -EVIRQ;
    }

    key = k_spin_lock(&vb->spinlock);
	lr_state = desc->virq_states;

	switch (lr_state) {
	case VIRQ_STATE_INVALID:
        desc->virq_flags |= VIRQ_PENDING_FLAG;
        if (!sys_dnode_is_linked(&desc->desc_node)) {
            sys_dlist_append(&vb->pending_irqs, &desc->desc_node);
            vb->virq_pending_counts++;
        }
        if (desc->virq_num < VM_LOCAL_VIRQ_NR) {
			/* get the src vcpu  */
            desc->src_cpu = get_current_vcpu_id();
        }
		break;
	case VIRQ_STATE_ACTIVE:
        desc->virq_flags |= VIRQ_ACTIVED_FLAG;
		/* if vm interrupt is not in active list */
        if (!sys_dnode_is_linked(&desc->desc_node)) {
            sys_dlist_append(&vb->pending_irqs, &desc->desc_node);
            vb->virq_pending_counts++;
        }
		break;
	case VIRQ_STATE_PENDING:
		break;
	case VIRQ_STATE_ACTIVE_AND_PENDING:
		break;
	}
    k_spin_unlock(&vb->spinlock, key);

	/**
	 * @Bug: Occur bug here: without judgement, wakeup_target_vcpu
	 * will introduce a pause vm error !
	 * When vcpu is not bind to current cpu, we should inform the
	 * dest pcpu. In this situation, vCPU may run on the other pcpus
	 * or it is in a idle states. So, we should consider all the
	 * situations.
	*/
	if(vcpu->work->vcpu_thread != _current){
		if(zvm_thread_active_elsewhere(vcpu->work->vcpu_thread)){
#if defined(CONFIG_SMP) &&  defined(CONFIG_SCHED_IPI_SUPPORTED)
			ZVM_LOG_INFO("Ready to send arch_sched_ipi \n");
			arch_sched_ipi();
#endif
		}else{
			wakeup_target_vcpu(vcpu, desc);
		}
	}

    return 0;
}

/**
 * @brief set sgi interrupt to vm, which usually used on vcpu
 * communication.
*/
static bool vgic_set_sgi2vcpu(struct vcpu *vcpu, struct virt_irq_desc *desc)
{
	return true;
}

static int vgic_gicd_mem_read(struct vcpu *vcpu, struct virt_gic_gicd *gicd,
                                uint32_t offset, uint64_t *v)
{
	int i, irq;
	uint32_t tmp_value;
	uint64_t *value = v;
	struct virt_irq_desc *desc;

	offset += GIC_DIST_BASE;
	switch (offset) {
		case GICD_CTLR:
			*value = vgic_sysreg_read32(gicd->gicd_regs_base, VGICD_CTLR) & ~(1 << 31);
			break;
		case GICD_TYPER:
			*value = vgic_sysreg_read32(gicd->gicd_regs_base, VGICD_TYPER);
			break;
		case GICD_STATUSR:
			*value = 0;
			break;
		case GICD_ISENABLERn...(GIC_DIST_BASE + 0x017c - 1):
			*value = 0;
			break;
		case GICD_ICENABLERn...(GIC_DIST_BASE + 0x01fc - 1):
			*value = 0;
			break;
		case (GIC_DIST_BASE + 0xffe8):
			*value = vgic_sysreg_read32(gicd->gicd_regs_base, VGICD_PIDR2);
			break;
		case GICD_ICFGRn...(GIC_DIST_BASE + 0x0cfc - 1):
			offset = (GIC_DIST_BASE+offset-GICD_ICFGRn) / 4;
			irq = 16 * offset;
			for (i = 0; i < 16; i++, irq++) {
				desc = vgic_get_virt_irq_desc(vcpu, irq);
				if(!desc) {
					continue;
				}
				tmp_value = desc->type;
				*value = *value | (tmp_value << i * 2);
			}
			break;
		default:
			*value = 0;
			break;
	}
	return 0;
}

static int vgic_gicd_mem_write(struct vcpu *vcpu, struct virt_gic_gicd *gicd,
                                uint32_t offset, uint64_t *v)
{
	int i, irq;
	uint32_t x, y, bit, t;
	uint32_t *value = (uint32_t *)v;
    k_spinlock_key_t key;

    key = k_spin_lock(&gicd->gicd_lock);
	offset += GIC_DIST_BASE;
	switch (offset) {
		case GICD_CTLR:
			vgic_sysreg_write32(*value, gicd->gicd_regs_base, VGICD_CTLR);
			break;
		case GICD_TYPER:
			break;
		case GICD_STATUSR:
			break;
		case GICD_ISENABLERn...(GIC_DIST_BASE + 0x017c - 1):
			x = (offset - GICD_ISENABLERn) / 4;
			y = x * 32;
			vgic_irq_test_and_set_bit(vcpu, y, value, 32, 1);
			break;
		case GICD_ICENABLERn...(GIC_DIST_BASE + 0x01fc - 1):
			x = (offset - GICD_ICENABLERn) / 4;
			y = x * 32;
			vgic_irq_test_and_set_bit(vcpu, y, value, 32, 0);
			break;
		case GICD_IPRIORITYRn...(GIC_DIST_BASE + 0x07f8 - 1):
			t = *value;
			x = (offset - GICD_IPRIORITYRn) / 4;
			y = x * 4 - 1;
			bit = (t & 0x000000ff);
			vgic_virq_set_priority(vcpu, y + 1, bit);
			bit = (t & 0x0000ff00) >> 8;
			vgic_virq_set_priority(vcpu, y + 2, bit);
			bit = (t & 0x00ff0000) >> 16;
			vgic_virq_set_priority(vcpu, y + 3, bit);
			bit = (t & 0xff000000) >> 24;
			vgic_virq_set_priority(vcpu, y + 4, bit);
			break;
		case GICD_ICFGRn...(GIC_DIST_BASE + 0x0cfc - 1):
			offset = (GIC_DIST_BASE+offset-GICD_ICFGRn) / 4;
			irq = 16 * offset;

			for (i = 0; i < 16; i++, irq++) {
				virt_irq_set_type(vcpu, irq, *value & 0x3);
				*value = *value >> 2;
			}
			break;
		default:
			break;
	}

	k_spin_unlock(&gicd->gicd_lock, key);

	return 0;
}

void vgic_irq_test_and_set_bit(struct vcpu *vcpu, uint32_t spi_nr_count, uint32_t *value,
						uint32_t bit_size, bool enable)
{
	int bit;
	uint32_t reg_mem_addr = (uint64_t)value;
	for (bit=0; bit<bit_size; bit++) {
		if (sys_test_bit(reg_mem_addr, bit)) {
			if (enable) {
				vgic_irq_enable(vcpu, spi_nr_count + bit);
			} else {
				/* TODO: add a situation for disable irq interrupt later */
				if (*value != 0xFFFFFFFF) {
					vgic_irq_disable(vcpu, spi_nr_count + bit);
				}
			}
		}
	}
}

void arch_vdev_irq_enable(struct vcpu *vcpu)
{
	uint32_t irq;
	struct vm *vm = vcpu->vm;
	struct virt_dev *vdev;
    struct  _dnode *d_node, *ds_node;

	SYS_DLIST_FOR_EACH_NODE_SAFE(&vm->vdev_list, d_node, ds_node) {

		/* Find the virt dev */
        vdev = CONTAINER_OF(d_node, struct virt_dev, vdev_node);

		/* enable spi interrupt */
		irq = vdev->hirq;

		if (irq > CONFIG_NUM_IRQS) {
			continue;
		}
		arm_gic_irq_enable(irq);
    }
}

void arch_vdev_irq_disable(struct vcpu *vcpu)
{
	uint32_t irq;
	struct vm *vm = vcpu->vm;
	struct virt_dev *vdev;
    struct  _dnode *d_node, *ds_node;

	SYS_DLIST_FOR_EACH_NODE_SAFE(&vm->vdev_list, d_node, ds_node) {

        /* Find the virt dev */
        vdev = CONTAINER_OF(d_node, struct virt_dev, vdev_node);

		/* disable spi interrupt */
		irq = vdev->hirq;

		if(irq > CONFIG_NUM_IRQS){
			continue;
		}

		arm_gic_irq_disable(irq);
    }
}

int vgicv3_raise_sgi(struct vcpu *vcpu, unsigned long sgi_value)
{
	int i;
	uint32_t sgi_id, sgi_mode;
	struct vcpu *target;
	struct vm *vm = vcpu->vm;

	sgi_id = (sgi_value & (0xf << 24)) >> 24;
	__ASSERT_NO_MSG(GIC_IS_SGI(sgi_id));

	sgi_mode = sgi_value & (1UL << 40) ? SGI_SIG_TO_OTHERS : SGI_SIG_TO_LIST;
	if (sgi_mode == SGI_SIG_TO_OTHERS) {
		for (i = 0; i < vm->vcpu_num; i++) {
			target = vm->vcpus[i];
			if (target == vcpu) {
				continue;
			}
			set_virq_to_vcpu(vcpu, sgi_id);
		}
		return 0;
	} else {
		ZVM_LOG_WARN("Unsupported sgi signal.");
		return -EVIRQ;
	}

	return 0;
}

int vgic_vdev_mem_read(struct virt_dev *vdev, uint64_t addr, uint64_t *value)
{
	uint32_t offset;
    int i;
	int type = TYPE_GIC_INVAILD;
	struct vcpu *vcpu = _current_vcpu;
	struct vgicv3_dev *vgic = (struct vgicv3_dev *)vdev->priv_data;
	struct virt_gic_gicd *gicd = &vgic->gicd;
	struct virt_gic_gicr *gicr = vgic->gicr[vcpu->vcpu_id];

	if ((addr >= gicd->gicd_base) && (addr < gicd->gicd_base + gicd->gicd_size)) {
		type = TYPE_GIC_GICD;
		offset = addr - gicd->gicd_base;
	} else {
		type = get_vcpu_gicr_type(gicr, addr, &offset);
		/* master vcpu may access other vcpu's gicr */
		if (type == TYPE_GIC_INVAILD){
			for (i = 0; i < vcpu->vm->vcpu_num; i++) {
				gicr = vgic->gicr[i];
				type = get_vcpu_gicr_type(gicr, addr, &offset);
				if (type != TYPE_GIC_INVAILD)
					break;
			}
		}
	}

	if (type == TYPE_GIC_INVAILD) {
		ZVM_LOG_WARN("Invaild gic type and address! The address: %llx \n", addr);
		return -EINVAL;
	}
	if (type != TYPE_GIC_GICD) {
        /* cannot access other vcpu redistribution */
		if (vcpu->vcpu_id != gicr->vcpu_id)
			return -EACCES;
	}

	switch (type) {
	case TYPE_GIC_GICD:
            return vgic_gicd_mem_read(vcpu, gicd, offset, value);
	case TYPE_GIC_GICR_RD:
			return vgic_gicrrd_mem_read(vcpu, gicr, offset, value);
	case TYPE_GIC_GICR_SGI:
			return vgic_gicrsgi_mem_read(vcpu, gicr, offset, value);
	case TYPE_GIC_GICR_VLPI:
			/* ignore vlpi register */
			*value = 0;
            return 0;
	default:
		ZVM_LOG_WARN("unsupport gic type %d\n", type);
		return -EINVAL;
	}

	return 0;
}

int vgic_vdev_mem_write(struct virt_dev *vdev, uint64_t addr, uint64_t *value)
{
    uint32_t offset;
    int i;
	int type = TYPE_GIC_INVAILD;
	struct vcpu *vcpu = _current_vcpu;
	struct vgicv3_dev *vgic = (struct vgicv3_dev *)vdev->priv_data;
	struct virt_gic_gicd *gicd = &vgic->gicd;
	struct virt_gic_gicr *gicr = vgic->gicr[vcpu->vcpu_id];

    if ((addr >= gicd->gicd_base) && (addr < gicd->gicd_base + gicd->gicd_size)) {
		type = TYPE_GIC_GICD;
		offset = addr - gicd->gicd_base;
	} else {
		type = get_vcpu_gicr_type(gicr, addr, &offset);
		/* master vcpu may access other vcpu's gicr */
		if (type == TYPE_GIC_INVAILD)
		for (i = 0; i < vcpu->vm->vcpu_num; i++) {
			gicr = vgic->gicr[i];
			type = get_vcpu_gicr_type(gicr, addr, &offset);
			if (type != TYPE_GIC_INVAILD)
				break;
		}
	}

	if (type == TYPE_GIC_INVAILD) {
		ZVM_LOG_WARN("Invaild gic type and address! The address: %llx \n", addr);
		return -EINVAL;
	}
	if (type != TYPE_GIC_GICD) {
        /* cannot access other vcpu redistribution */
		if (vcpu->vcpu_id != gicr->vcpu_id)
			return -EACCES;
	}

	switch (type) {
		case TYPE_GIC_GICD:
				return vgic_gicd_mem_write(vcpu, gicd, offset, value);
		case TYPE_GIC_GICR_RD:
				return vgic_gicrrd_mem_write(vcpu, gicr, offset, value);
		case TYPE_GIC_GICR_SGI:
				return vgic_gicrsgi_mem_write(vcpu, gicr, offset, value);
		case TYPE_GIC_GICR_VLPI:
				return 0; //ignore on this stage. @TODO
		default:
			ZVM_LOG_WARN("Unsupport gic type %d\n", type);
			return -EINVAL;
	}

	return 0;
}

int set_virq_to_vcpu(struct vcpu *vcpu, uint32_t virq_num)
{
    struct virt_irq_desc *desc;

    desc = vgic_get_virt_irq_desc(vcpu, virq_num);
    if(!desc){
        ZVM_LOG_WARN("Get virt irq desc error here!");
        return -EVIRQ;
    }

    return vgic_set_virq(vcpu, desc);
}

int set_virq_to_vm(struct vm *vm, uint32_t virq_num)
{
    uint32_t ret = 0;
    struct virt_irq_desc *desc;
    struct vcpu *vcpu, *target_vcpu;
    vcpu = vm->vcpus[DEFAULT_VCPU];

    if (virq_num < VM_LOCAL_VIRQ_NR) {
        desc = &vcpu->virq_block.vcpu_virt_irq_desc[virq_num];
    } else if (virq_num <= VM_GLOBAL_VIRQ_NR) {
        desc = &vm->vm_irq_block.vm_virt_irq_desc[virq_num - VM_LOCAL_VIRQ_NR];
    } else {
        ZVM_LOG_WARN("The spi num that ready to allocate is too big.");
        return -ENODEV;
    }

    target_vcpu = vm->vcpus[desc->vcpu_id];
    ret = vgic_set_virq(target_vcpu, desc);
    if (ret >= 0) {
		return VM_IRQ_TO_VM_SUCCESS;
	}

	return ret;
}

int virt_irq_sync_vgic(struct vcpu *vcpu)
{
	uint8_t lr_state;
	uint64_t elrsr, eisr;
	k_spinlock_key_t key;
	struct virt_irq_desc *desc;
    struct _dnode *d_node, *ds_node;
	struct vcpu_virt_irq_block *vb = &vcpu->virq_block;

	key = k_spin_lock(&vb->spinlock);
	if (vb->virq_pending_counts == 0) {
		k_spin_unlock(&vb->spinlock, key);
		return 0;
	}

	/* Get the maintain or valid irq */
	elrsr = read_elrsr_el2();
	eisr = read_eisr_el2();
	elrsr |= eisr;
	elrsr &= vcpu->arch->list_regs_map;

    SYS_DLIST_FOR_EACH_NODE_SAFE(&vb->active_irqs, d_node, ds_node) {
        desc = CONTAINER_OF(d_node, struct virt_irq_desc, desc_node);
		/* A valid interrupt? store it again! */
		if(!VGIC_ELRSR_REG_TEST(desc->id, elrsr)){
			continue;
		}

        lr_state = gicv3_get_lr_state(vcpu, desc);
		switch (lr_state) {
		/* vm interrupt is done or need pending again when it is active  */
		case VIRQ_STATE_ACTIVE:
			/* if this sync is not irq trap */
			if(vcpu->exit_type != ARM_VM_EXCEPTION_IRQ){
				desc->virq_states = lr_state;
				break;
			}
		case VIRQ_STATE_INVALID:
			gicv3_update_lr(vcpu, desc, ACTION_CLEAR_VIRQ, 0);
			vcpu->arch->hcr_el2 &= ~(uint64_t)HCR_VI_BIT;
			sys_dlist_remove(&desc->desc_node);
			/* if software irq is still triggered */
			if (desc->vdev_trigger) {
				/* This means vm interrupt is done, but host interrupt is pending */
				sys_dlist_append(&vb->pending_irqs, &desc->desc_node);
			}
			vb->virq_pending_counts--;
		/* vm interrupt is still pending, no need to inject again. */
		case VIRQ_STATE_PENDING:
		case VIRQ_STATE_ACTIVE_AND_PENDING:
			desc->virq_states = lr_state;
			break;
		}
    }
	k_spin_unlock(&vb->spinlock, key);

	return 0;
}

int virt_irq_flush_vgic(struct vcpu *vcpu)
{
	int ret;
	k_spinlock_key_t key;
	struct virt_irq_desc *desc;
    struct _dnode *d_node, *ds_node;
	struct vcpu_virt_irq_block *vb = &vcpu->virq_block;

	key = k_spin_lock(&vb->spinlock);
	if (vb->virq_pending_counts == 0) {
		/* No pending irq, just return! */
		k_spin_unlock(&vb->spinlock, key);
		return 0;
	}

	/* no idle list register */
	if(vcpu->arch->list_regs_map == ((1<<VGIC_TYPER_LR_NUM) -1) ){
		k_spin_unlock(&vb->spinlock, key);
		ZVM_LOG_WARN("There is no idle list register! ");
		return 0;
	}

    SYS_DLIST_FOR_EACH_NODE_SAFE(&vb->pending_irqs, d_node, ds_node) {
        desc = CONTAINER_OF(d_node, struct virt_irq_desc, desc_node);

		/* if there the vm interrupt is not deactived, avoid inject it again */
		if(!(desc->virq_states==VIRQ_STATE_INVALID || desc->virq_states==VIRQ_STATE_ACTIVE)){
			continue;
		}

        if (desc->virq_flags & VIRQ_PENDING_FLAG || desc->virq_flags & VIRQ_ACTIVED_FLAG) {
			switch (VGIC_VIRQ_LEVEL_SORT(desc->virq_num)) {
			case VGIC_VIRQ_IN_SGI:
				vgic_set_sgi2vcpu(vcpu, desc);
			case VGIC_VIRQ_IN_PPI:
			default:
				break;
			}
			desc->id = gicv3_get_idle_lr(vcpu);
			if(desc->id < 0){
				ZVM_LOG_WARN("No idle list register for virq: %d. \n", desc->id);
				break;
			}
            ret = gicv3_inject_virq(vcpu, desc);
            if (ret) {
				k_spin_unlock(&vb->spinlock, key);
                return ret;
            }
            desc->virq_states = VIRQ_STATE_PENDING;
            desc->virq_flags &= (uint32_t)~VIRQ_PENDING_FLAG;
            sys_dlist_remove(&desc->desc_node);
            sys_dlist_append(&vb->active_irqs, &desc->desc_node);
        }else {
			ZVM_LOG_WARN("Some thing wrong, virq-id %d is not pending but in the list. \n", desc->id);
			gicv3_update_lr(vcpu, desc, ACTION_CLEAR_VIRQ, 0);
			desc->id = VM_INVALID_DESC_ID;
			sys_dlist_remove(&desc->desc_node);
		}
    }
	k_spin_unlock(&vb->spinlock, key);

	return 0;
}

struct virt_irq_desc *get_virt_irq_desc(struct vcpu *vcpu, uint32_t virq)
{
	return vgic_get_virt_irq_desc(vcpu, virq);
}

int vm_intctrl_vdev_create(struct vm *vm)
{
	int ret = 0;
	const struct device *dev = DEVICE_DT_GET(DT_ALIAS(vmvgic));

	if(((const struct virt_device_api * const)(dev->api))->init_fn){
		((const struct virt_device_api * const)(dev->api))->init_fn(dev, vm, NULL);
	}else{
		ZVM_LOG_ERR("No gic device api! \n");
		return -ENODEV;
	}

	return ret;
}
