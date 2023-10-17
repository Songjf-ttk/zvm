/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
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
#include <virtualization/arm/vgic_v3.h>
#include <virtualization/arm/vgic_common.h>
#include <virtualization/zvm.h>
#include <virtualization/vm_irq.h>
#include <virtualization/vm_console.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

static struct virt_irq_desc *vgic_get_virt_irq_desc(struct vcpu *vcpu, uint32_t virq)
{
	struct vm *vm = vcpu->vm;

    /* sgi virq num */
	if (virq < VM_LOCAL_VIRQ_NR) {
        return &vcpu->virq_struct->vcpu_virt_irq_desc[virq];
    }

    /* spi virq num */
    if((virq >= VM_LOCAL_VIRQ_NR) && (virq < VM_SPI_VIRQ_NR + VM_LOCAL_VIRQ_NR)) {
        return &VGIC_DATA_VID(vm->vm_irq_block_data)[virq - VM_LOCAL_VIRQ_NR];
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

    desc->virq_flags |= VIRT_IRQ_ENABLED;

	if (virt_irq > VM_LOCAL_VIRQ_NR) {
		if (desc->virq_flags & VIRT_IRQ_HW) {
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

	desc->virq_flags &= ~VIRT_IRQ_ENABLED;

	if (virt_irq > VM_LOCAL_VIRQ_NR) {
		if (desc->virq_flags & VIRT_IRQ_HW) {
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
		if (desc->virq_flags & VIRT_IRQ_HW) {
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

static int virt_irq_set_priority(struct vcpu *vcpu, uint32_t virt_irq, int prio)
{
	struct virt_irq_desc *desc;

	desc = vgic_get_virt_irq_desc(vcpu, virt_irq);
	if (!desc) {
		return -ENOENT;
	}
	desc->prio = prio;

	return 0;
}

static int set_virt_irq(struct vcpu *vcpu, struct virt_irq_desc *desc)
{
    k_spinlock_key_t key;
    struct virq_struct *virq_struct = vcpu->virq_struct;

    if (!is_vm_irq_valid(vcpu->vm, desc->virq_flags)) {
        ZVM_LOG_WARN("VM can not recieve virq signal, \
						VM's name: %s.", vcpu->vm->vm_name);
        return -EVIRQ;
    }

    key = k_spin_lock(&virq_struct->spinlock);
    if (!(desc->virq_flags & VIRQ_PENDING_FLAG)) {
        desc->virq_flags |= VIRQ_PENDING_FLAG;
        if (!sys_dnode_is_linked(&desc->desc_node)) {
            sys_dlist_append(&virq_struct->pending_irqs, &desc->desc_node);
            virq_struct->virq_act_counts++;
        }
        if (desc->virq_num < VM_LOCAL_VIRQ_NR) {
            desc->src_cpu = get_current_vcpu_id();
        }
    }
    k_spin_unlock(&virq_struct->spinlock, key);

	/*occur bug here: without judgement, wakeup_target_vcpu will 
	  introduce a pause vm error !*/
	if(vcpu->work->vcpu_thread != _current)
    	wakeup_target_vcpu(vcpu, desc);

    return 0;
}

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
			*value = gicd->gicd_ctlr & ~(1 << 31);
			break;
		case GICD_TYPER:
			*value = gicd->gicd_typer;
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
			*value = gicd->gicd_pidr2;
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
			gicd->gicd_ctlr = *value;
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
			virt_irq_set_priority(vcpu, y + 1, bit);
			bit = (t & 0x0000ff00) >> 8;
			virt_irq_set_priority(vcpu, y + 2, bit);
			bit = (t & 0x00ff0000) >> 16;
			virt_irq_set_priority(vcpu, y + 3, bit);
			bit = (t & 0xff000000) >> 24;
			virt_irq_set_priority(vcpu, y + 4, bit);
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
						uint32_t bit_size, int enable)
{
	int bit;
	uint32_t mem_addr = (uint64_t)value;
	for (bit=0; bit<bit_size; bit++) {
		if (sys_test_bit(mem_addr, bit)) {
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

int vgic_vdev_mem_read(struct virt_dev *vdev, arch_commom_regs_t *regs,
                            uint64_t addr, uint64_t *value)
{
	ARG_UNUSED(regs);
	uint32_t offset;
    int i;
	int type = TYPE_GIC_INVAILD;
	struct vcpu *vcpu = _current_vcpu;
	struct vgicv3_dev *vgic = CONTAINER_OF(vdev, struct vgicv3_dev, v_dev);
	struct virt_gic_gicd *gicd = &vgic->gicd;
	struct virt_gic_gicr *gicr = vgic->gicr[vcpu->vcpu_id];

	if ((addr >= gicd->addr_base) && (addr < gicd->addr_base + gicd->addr_size)) {
		type = TYPE_GIC_GICD;
		offset = addr - gicd->addr_base;
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
            return vgic_gicd_mem_read(vcpu, gicd, offset, value);
	case TYPE_GIC_GICR_RD:
			return vgic_gicrrd_mem_read(vcpu, gicr, offset, value);
	case TYPE_GIC_GICR_SGI:
			return vgic_gicrsgi_mem_read(vcpu, gicr, offset, value);
	case TYPE_GIC_GICR_VLPI:
			*value = 0;
            return 0; 	//ignore on this stage. @TODO
	default:
		ZVM_LOG_WARN("unsupport gic type %d\n", type);
		return -EINVAL;
	}

	return 0;
}

int vgic_vdev_mem_write(struct virt_dev *vdev, arch_commom_regs_t *regs,
                            uint64_t addr, uint64_t *value)
{
	ARG_UNUSED(regs);
    uint32_t offset;
    int i;
	int type = TYPE_GIC_INVAILD;
	struct vcpu *vcpu = _current_vcpu;
	struct vgicv3_dev *vgic = CONTAINER_OF(vdev, struct vgicv3_dev, v_dev); 
	struct virt_gic_gicd *gicd = &vgic->gicd;
	struct virt_gic_gicr *gicr = vgic->gicr[vcpu->vcpu_id];

    if ((addr >= gicd->addr_base) && (addr < gicd->addr_base + gicd->addr_size)) {
		type = TYPE_GIC_GICD;
		offset = addr - gicd->addr_base;
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

    return set_virt_irq(vcpu, desc);
}

int set_virq_to_vm(struct vm *vm, uint32_t virq_num)
{
    uint32_t ret = 0;
    struct virt_irq_desc *desc;
    struct vcpu *vcpu, *target_vcpu;
    vcpu = vm->vcpus[DEFAULT_VCPU];

    if (!vm) {
        ZVM_LOG_WARN("VM struct not exit here!");
        return -ENODEV;
    }

    /* get desc from vm or vcpu struct */
    if (virq_num < VM_LOCAL_VIRQ_NR) {
        desc = &vcpu->virq_struct->vcpu_virt_irq_desc[virq_num];
    } else if (virq_num <= VM_SPI_VIRQ_NR + VM_LOCAL_VIRQ_NR) {
        desc = VGIC_DATA_VID(vm->vm_irq_block_data);
        desc = &desc[virq_num - VM_LOCAL_VIRQ_NR];
    } else {
        ZVM_LOG_WARN("The spi num that ready to allocate is too big.");
        return -ENODEV;
    }

    target_vcpu = vm->vcpus[desc->vcpu_id];
    ret = set_virt_irq(target_vcpu, desc);
    if (ret >= 0) {
		return VM_IRQ_TO_VM_SUCCESS;
	}

	return ret;
}

int virt_irq_sync_vgic(struct vcpu *vcpu)
{
	uint8_t lr_status;
	k_spinlock_key_t key;
	struct virt_irq_desc *desc;
    struct _dnode *d_node, *ds_node;
	struct virq_struct *virq_struct = vcpu->virq_struct;

	key = k_spin_lock(&virq_struct->spinlock);
	if (virq_struct->virq_act_counts == 0) {
		k_spin_unlock(&virq_struct->spinlock, key);
		return 0;
	}

    SYS_DLIST_FOR_EACH_NODE_SAFE(&virq_struct->active_irqs, d_node, ds_node) {
        desc = CONTAINER_OF(d_node, struct virt_irq_desc, desc_node);
        lr_status = gicv3_get_lr_state(vcpu, desc);

		if (lr_status == VIRT_IRQ_STATE_INACTIVE) {
			if (desc->virq_flags & VIRQ_PENDING_FLAG) {
				gicv3_update_lr(vcpu, desc, ACTION_CLEAR_VIRQ);
				sys_dlist_remove(&desc->desc_node);
				sys_dlist_append(&virq_struct->pending_irqs, &desc->desc_node);
			} else {
				gicv3_update_lr(vcpu, desc, ACTION_CLEAR_VIRQ);
				desc->virq_states = VIRT_IRQ_STATE_INACTIVE;
				desc->id = VIRT_IRQ_INVALID_ID;
				sys_dlist_remove(&desc->desc_node);
				virq_struct->virq_act_counts--;
			}
		} else {
			desc->virq_states = lr_status;
		}
    }
	k_spin_unlock(&virq_struct->spinlock, key);

	return 0;
}

int virt_irq_flush_vgic(struct vcpu *vcpu)
{
	int ret;
	k_spinlock_key_t key;
	struct virt_irq_desc *desc;
    struct _dnode *d_node, *ds_node;
	struct virq_struct *virq_struct = vcpu->virq_struct;

	key = k_spin_lock(&virq_struct->spinlock);
	if (virq_struct->virq_act_counts == 0) {
		k_spin_unlock(&virq_struct->spinlock, key);
		return 0;
	}
    SYS_DLIST_FOR_EACH_NODE_SAFE(&virq_struct->pending_irqs, d_node, ds_node) {
        desc = CONTAINER_OF(d_node, struct virt_irq_desc, desc_node);

        if (desc->virq_flags & VIRQ_PENDING_FLAG) {

			switch (VGIC_VIRQ_LEVEL_SORT(desc->virq_num)) {
			/*TODO: Add muti-vcpu support later.*/
			case VGIC_VIRQ_IN_SGI:
				vgic_set_sgi2vcpu(vcpu,desc);
			case VGIC_VIRQ_IN_PPI:
			default:
				break;
			}
            if (desc->id == VIRT_IRQ_INVALID_ID) {
				desc->id = read_sysreg(ICH_VTR_EL2) & 0x1f;
            }
            ret = gicv3_inject_virq(vcpu, desc);
            if (ret) {
                return ret;
            }
            desc->virq_states = VIRT_IRQ_STATE_PENDING;
            desc->virq_flags &= (uint32_t)~VIRQ_PENDING_FLAG;
            sys_dlist_remove(&desc->desc_node);
            sys_dlist_append(&virq_struct->active_irqs, &desc->desc_node);
            continue;
        }
        ZVM_LOG_WARN("Some thing wrong, virq-id %d is not pending but in the list. \n", desc->id);
		gicv3_update_lr(vcpu, desc, ACTION_CLEAR_VIRQ);
		desc->id = VIRT_IRQ_INVALID_ID;
        sys_dlist_remove(&desc->desc_node);
    }
	k_spin_unlock(&virq_struct->spinlock, key);

	return 0;
}

struct virt_irq_desc *get_virt_irq_desc(struct vcpu *vcpu, uint32_t virq)
{
	return vgic_get_virt_irq_desc(vcpu, virq);
}

int vm_irq_ctrlblock_create(struct device *unused, struct vm *vm)
{
	return vgicv3_ctrlblock_create(unused, vm);
}

int vm_virq_desc_init(struct vm *vm, void *args)
{
    ARG_UNUSED(args);
    int ret = 0;
    int i;
    struct virt_irq_desc *desc;

    for (i = 0; i < VM_SPI_VIRQ_NR; i++) {
        desc = &VGIC_DATA_VID(vm->vm_irq_block_data)[i];

        desc->virq_flags = VIRT_IRQ_NOUSED;
        desc->vcpu_id =  DEFAULT_VCPU;
        desc->vm_id = vm->vmid;
        desc->virq_num = i;
        desc->pirq_num = i;
        desc->id = VIRT_IRQ_INVALID_ID;
        desc->virq_states = VIRT_IRQ_STATE_INACTIVE;

        sys_dnode_init(&(desc->desc_node));
    }

    return ret;
}

int zvm_arch_vgic_init(void *op)
{
#ifdef CONFIG_GIC_V3
	return arch_vgicv3_init(op);
#else
	/*No gicv3 init!*/
	return -ENODEV;
#endif
}
