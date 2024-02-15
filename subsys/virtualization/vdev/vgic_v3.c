/*
 * Copyright 2021-2022 HNU-ESNL
 * Copyright 2023 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <kernel_structs.h>
#include <init.h>
#include <arch/cpu.h>
#include <arch/arm64/lib_helpers.h>
#include <arch/common/sys_bitops.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>
#include <drivers/interrupt_controller/gic.h>
#include <logging/log.h>
#include <../drivers/interrupt_controller/intc_gicv3_priv.h>
#include <virtualization/arm/cpu.h>
#include <virtualization/vdev/vgic_v3.h>
#include <virtualization/vdev/vgic_common.h>
#include <virtualization/zvm.h>
#include <virtualization/vm_irq.h>
#include <virtualization/vm_console.h>
#include <virtualization/vm_dev.h>
#include <virtualization/vdev/virt_device.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define DEV_CFG(dev) \
	((const struct virt_device_config * const)(dev)->config)

#define DEV_VGICV3(dev) \
	((const struct gicv3_vdevice * const)(DEV_CFG(dev)->device_config))

/**
 * @brief load list register for vcpu interface.
 */
static void vgicv3_lrs_load(struct gicv3_vcpuif_ctxt *ctxt)
{
    uint32_t rg_cout = VGIC_TYPER_LR_NUM;

    if (rg_cout > VGIC_TYPER_LR_NUM) {
        ZVM_LOG_WARN("System list registers do not support! \n");
        return;
    }

	switch (rg_cout) {
	case 8:
		write_sysreg(ctxt->ich_lr7_el2, ICH_LR7_EL2);
	case 7:
		write_sysreg(ctxt->ich_lr6_el2, ICH_LR6_EL2);
	case 6:
		write_sysreg(ctxt->ich_lr5_el2, ICH_LR5_EL2);
	case 5:
		write_sysreg(ctxt->ich_lr4_el2, ICH_LR4_EL2);
	case 4:
		write_sysreg(ctxt->ich_lr3_el2, ICH_LR3_EL2);
	case 3:
		write_sysreg(ctxt->ich_lr2_el2, ICH_LR2_EL2);
	case 2:
		write_sysreg(ctxt->ich_lr1_el2, ICH_LR1_EL2);
	case 1:
		write_sysreg(ctxt->ich_lr0_el2, ICH_LR0_EL2);
		break;
	default:
		break;
	}
}

static void vgicv3_prios_load(struct gicv3_vcpuif_ctxt *ctxt)
{
    uint32_t rg_cout = VGIC_TYPER_PRIO_NUM;

    switch (rg_cout) {
	case 7:
		write_sysreg(ctxt->ich_ap0r2_el2, ICH_AP0R2_EL2);
		write_sysreg(ctxt->ich_ap1r2_el2, ICH_AP1R2_EL2);
	case 6:
		write_sysreg(ctxt->ich_ap0r1_el2, ICH_AP0R1_EL2);
		write_sysreg(ctxt->ich_ap1r1_el2, ICH_AP1R1_EL2);
	case 5:
		write_sysreg(ctxt->ich_ap0r0_el2, ICH_AP0R0_EL2);
		write_sysreg(ctxt->ich_ap1r0_el2, ICH_AP1R0_EL2);
		break;
	default:
		ZVM_LOG_ERR("Load prs error");
	}
}

static void vgicv3_ctrls_load(struct gicv3_vcpuif_ctxt *ctxt)
{
    write_sysreg(ctxt->icc_sre_el1, ICC_SRE_EL1);
	write_sysreg(ctxt->icc_ctlr_el1, ICC_CTLR_EL1);

	write_sysreg(ctxt->ich_vmcr_el2, ICH_VMCR_EL2);
	write_sysreg(ctxt->ich_hcr_el2, ICH_HCR_EL2);
}

static void vgicv3_lrs_save(struct gicv3_vcpuif_ctxt *ctxt)
{
	uint32_t rg_cout = VGIC_TYPER_LR_NUM;

    if (rg_cout > VGIC_TYPER_LR_NUM) {
        ZVM_LOG_WARN("System list registers do not support! \n");
        return;
    }

	switch (rg_cout) {
	case 8:
		ctxt->ich_lr7_el2 = read_sysreg(ICH_LR7_EL2);
	case 7:
		ctxt->ich_lr6_el2 = read_sysreg(ICH_LR6_EL2);
	case 6:
		ctxt->ich_lr5_el2 = read_sysreg(ICH_LR5_EL2);
	case 5:
		ctxt->ich_lr4_el2 = read_sysreg(ICH_LR4_EL2);
	case 4:
		ctxt->ich_lr3_el2 = read_sysreg(ICH_LR3_EL2);
	case 3:
		ctxt->ich_lr2_el2 = read_sysreg(ICH_LR2_EL2);
	case 2:
		ctxt->ich_lr1_el2 = read_sysreg(ICH_LR1_EL2);
	case 1:
		ctxt->ich_lr0_el2 = read_sysreg(ICH_LR0_EL2);
		break;
	default:
		break;
	}
}

static void vgicv3_lrs_init(void)
{
    uint32_t rg_cout = VGIC_TYPER_LR_NUM;

    if (rg_cout > VGIC_TYPER_LR_NUM) {
        ZVM_LOG_WARN("System list registers do not support! \n");
        return;
    }

	rg_cout = rg_cout>8 ? 8 : rg_cout;

	switch (rg_cout) {
	case 8:
		write_sysreg(0, ICH_LR7_EL2);
	case 7:
		write_sysreg(0, ICH_LR6_EL2);
	case 6:
		write_sysreg(0, ICH_LR5_EL2);
	case 5:
		write_sysreg(0, ICH_LR4_EL2);
	case 4:
		write_sysreg(0, ICH_LR3_EL2);
	case 3:
		write_sysreg(0, ICH_LR2_EL2);
	case 2:
		write_sysreg(0, ICH_LR1_EL2);
	case 1:
		write_sysreg(0, ICH_LR0_EL2);
		break;
	default:
		break;
	}
}

static void vgicv3_prios_save(struct gicv3_vcpuif_ctxt *ctxt)
{
    uint32_t rg_cout = VGIC_TYPER_PRIO_NUM;

	switch (rg_cout) {
	case 7:
		ctxt->ich_ap0r2_el2 = read_sysreg(ICH_AP0R2_EL2);
		ctxt->ich_ap1r2_el2 = read_sysreg(ICH_AP1R2_EL2);
	case 6:
		ctxt->ich_ap0r1_el2 = read_sysreg(ICH_AP0R1_EL2);
		ctxt->ich_ap1r1_el2 = read_sysreg(ICH_AP1R1_EL2);
	case 5:
		ctxt->ich_ap0r0_el2 = read_sysreg(ICH_AP0R0_EL2);
		ctxt->ich_ap1r0_el2 = read_sysreg(ICH_AP1R0_EL2);
		break;
	default:
		ZVM_LOG_ERR(" Set ich_ap priority failed. \n");
	}
}

static void vgicv3_ctrls_save(struct gicv3_vcpuif_ctxt *ctxt)
{
    ctxt->icc_sre_el1 = read_sysreg(ICC_SRE_EL1);
	ctxt->icc_ctlr_el1 = read_sysreg(ICC_CTLR_EL1);

	ctxt->ich_vmcr_el2 = read_sysreg(ICH_VMCR_EL2);
	ctxt->ich_hcr_el2 = read_sysreg(ICH_HCR_EL2);
}


int gicv3_inject_virq(struct vcpu *vcpu, struct virt_irq_desc *desc)
{
	uint64_t value = 0;
	struct gicv3_list_reg *lr = (struct gicv3_list_reg *)&value;

	if (desc->id >= VGIC_TYPER_LR_NUM) {
		ZVM_LOG_WARN("invalid virq id %d, It is used by other device! \n", desc->id);
		return -EINVAL;
	}

	/* List register is not activated. */
	if (VGIC_LIST_REGS_TEST(desc->id, vcpu)) {
		value = gicv3_read_lr(desc->id);
		lr = (struct gicv3_list_reg *)&value;
		if (lr->vINTID == desc->virq_num) {
			desc->virq_flags |= VIRQ_PENDING_FLAG;
		}
	}

	lr->vINTID = desc->virq_num;
	lr->pINTID = desc->pirq_num;
	lr->priority = desc->prio;
	lr->group = LIST_REG_GROUP1;
	lr->hw = LIST_REG_HW_VIRQ;
	lr->state = VIRQ_STATE_PENDING;
	gicv3_update_lr(vcpu, desc, ACTION_SET_VIRQ, value);
	return 0;
}

int vgic_gicrsgi_mem_read(struct vcpu *vcpu, struct virt_gic_gicr *gicr,
                                uint32_t offset, uint64_t *v)
{
	uint32_t *value = (uint32_t *)v;

	switch (offset) {
	case GICR_SGI_CTLR:
		*value = vgic_sysreg_read32(gicr->gicr_sgi_reg_base, VGICR_CTLR) & ~(1 << 31);
		break;
	case GICR_SGI_ISENABLER:
		*value = vgic_sysreg_read32(gicr->gicr_sgi_reg_base, VGICR_ISENABLER0);
		break;
	case GICR_SGI_ICENABLER:
		*value = vgic_sysreg_read32(gicr->gicr_sgi_reg_base, VGICR_ICENABLER0);
		break;
	case GICR_SGI_PENDING:
		vgic_sysreg_write32(*value, gicr->gicr_sgi_reg_base, VGICR_SGI_PENDING);
		break;
	case GICR_SGI_PIDR2:
		*value = (0x03 << 4);
		break;
	default:
		*value = 0;
		break;
	}

	return 0;
}

int vgic_gicrsgi_mem_write(struct vcpu *vcpu, struct virt_gic_gicr *gicr, uint32_t offset, uint64_t *v)
{
	uint32_t *value = (uint32_t *)v;
	uint32_t mem_addr = (uint64_t)v;
	int bit;

	switch (offset) {
	case GICR_SGI_ISENABLER:
		vgic_irq_test_and_set_bit(vcpu, 0, value, 32, 1);
		for (bit = 0; bit < 32; bit++) {
			if (sys_test_bit(mem_addr, bit)) {
				vgic_sysreg_write32(vgic_sysreg_read32(gicr->gicr_sgi_reg_base, VGICR_ISENABLER0) | BIT(bit),\
				 gicr->gicr_sgi_reg_base, VGICR_ISENABLER0);
			}
		}
		break;
	case GICR_SGI_ICENABLER:
		vgic_irq_test_and_set_bit(vcpu, 0, value, 32, 0);
		for(bit = 0; bit < 32; bit++) {
			if (sys_test_bit(mem_addr, bit)) {
				vgic_sysreg_write32(vgic_sysreg_read32(gicr->gicr_sgi_reg_base, VGICR_ICENABLER0) & ~BIT(bit),\
				 gicr->gicr_sgi_reg_base, VGICR_ICENABLER0);
			}
		}
		break;
	case GICR_SGI_PENDING:
		/* clear pending state */
		for(bit = 0; bit < 32; bit++) {
			if (sys_test_bit(mem_addr, bit)) {
				sys_write32(BIT(bit), GIC_RDIST_BASE + GICR_SGI_BASE_OFF + GICR_SGI_PENDING);
				vgic_sysreg_write32(~BIT(bit), gicr->gicr_sgi_reg_base, VGICR_SGI_PENDING);
			}
		}
		break;
	default:
		*value = 0;
		break;
	}

	return 0;
}

int vgic_gicrrd_mem_read(struct vcpu *vcpu, struct virt_gic_gicr *gicr, uint32_t offset, uint64_t *v)
{
	uint32_t *value = (uint32_t *)v;

	/* consider multiple cpu later, Now just return 0 */
	switch (offset) {
	case 0xffe8:
		*value = vgic_sysreg_read32(gicr->gicr_rd_reg_base, VGICR_PIDR2);
		break;
	case GICR_CTLR:
		vgic_sysreg_write32(*value, gicr->gicr_rd_reg_base, VGICR_CTLR);
		break;
	case GICR_TYPER:
		*value = vgic_sysreg_read32(gicr->gicr_rd_reg_base, VGICR_TYPER);
		*(value+1) = vgic_sysreg_read32(gicr->gicr_rd_reg_base, VGICR_TYPER+0x4);
		break;
	case 0x000c:
		*value = vgic_sysreg_read32(gicr->gicr_rd_reg_base, VGICR_TYPER+0x4);
		break;
	default:
		*value = 0;
		break;
	}

	return 0;
}

int vgic_gicrrd_mem_write(struct vcpu *vcpu, struct virt_gic_gicr *gicr, uint32_t offset, uint64_t *v)
{
	return 0;
}

int get_vcpu_gicr_type(struct virt_gic_gicr *gicr,
		uint32_t addr, uint32_t *offset)
{
	int i;
	uint32_t vcpu_id = gicr->vcpu_id;

	/* master core can access all the other core's gicr */
	if (vcpu_id == 0) {
		for(i=0; i<VGIC_RDIST_SIZE/VGIC_RD_SGI_SIZE + 1; i++){
			if ((addr >= gicr->gicr_sgi_base + i*VGIC_RD_SGI_SIZE) &&
				(addr < gicr->gicr_sgi_base + i*VGIC_RD_SGI_SIZE + gicr->gicr_sgi_size)) {
				*offset = addr - (gicr->gicr_sgi_base + i*VGIC_RD_SGI_SIZE);
				return TYPE_GIC_GICR_SGI;
			}

			if ((addr >= gicr->gicr_rd_base + i*VGIC_RD_SGI_SIZE) &&
				(addr < (gicr->gicr_rd_base +  i*VGIC_RD_SGI_SIZE + gicr->gicr_rd_size))) {
				*offset = addr - (gicr->gicr_rd_base + i*VGIC_RD_SGI_SIZE);
				return TYPE_GIC_GICR_RD;
			}
		}
	} else {
		if ((addr >= gicr->gicr_sgi_base + vcpu_id*VGIC_RD_SGI_SIZE) &&
			(addr < (gicr->gicr_sgi_base + vcpu_id*VGIC_RD_SGI_SIZE + gicr->gicr_sgi_size))) {
			*offset = addr - (gicr->gicr_sgi_base + vcpu_id*VGIC_RD_SGI_SIZE);
			return TYPE_GIC_GICR_SGI;
		}

		if ((addr >= gicr->gicr_rd_base + vcpu_id*VGIC_RD_SGI_SIZE) &&
			(addr < (gicr->gicr_rd_base +  vcpu_id*VGIC_RD_SGI_SIZE + gicr->gicr_rd_size))) {
			*offset = addr - (gicr->gicr_rd_base + vcpu_id*VGIC_RD_SGI_SIZE);
			return TYPE_GIC_GICR_RD;
		}
	}

	return TYPE_GIC_INVAILD;
}

int vgicv3_state_load(struct vcpu *vcpu, struct gicv3_vcpuif_ctxt *ctxt)
{
    vgicv3_lrs_load(ctxt);
    vgicv3_prios_load(ctxt);
    vgicv3_ctrls_load(ctxt);

	arch_vdev_irq_enable(vcpu);
	return 0;
}

int vgicv3_state_save(struct vcpu *vcpu, struct gicv3_vcpuif_ctxt *ctxt)
{
    vgicv3_lrs_save(ctxt);
    vgicv3_prios_save(ctxt);
    vgicv3_ctrls_save(ctxt);

	arch_vdev_irq_disable(vcpu);
	return 0;
}

/**
 * @brief Gic vcpu interface init .
 */
int vcpu_gicv3_init(struct gicv3_vcpuif_ctxt *ctxt)
{

    ctxt->icc_sre_el1 = 0x07;
	ctxt->icc_ctlr_el1 = read_sysreg(ICC_CTLR_EL1);
	ctxt->icc_ctlr_el1 |= 0x02;

    ctxt->ich_vmcr_el2 = GICH_VMCR_VENG1 | GICH_VMCR_DEFAULT_MASK;
    ctxt->ich_hcr_el2 = GICH_HCR_EN;

    return 0;
}

static int vdev_gicv3_init(struct vm *vm, struct vgicv3_dev *gicv3_vdev, uint32_t gicd_base, uint32_t gicd_size,
                            uint32_t gicr_base, uint32_t gicr_size)
{
	int i = 0;
    uint32_t spi_num;
    uint64_t tmp_typer = 0;
    struct virt_gic_gicd *gicd = &gicv3_vdev->gicd;
    struct virt_gic_gicr *gicr;

	gicd->gicd_base = gicd_base;
    gicd->gicd_size = gicd_size;
	gicd->gicd_regs_base = k_malloc(gicd->gicd_size);
	if(!gicd->gicd_regs_base){
		return -EMMAO;
	}
	memset(gicd->gicd_regs_base, 0, gicd_size);

	/* GICD PIDR2 */
	vgic_sysreg_write32(0x3<<4, gicd->gicd_regs_base, VGICD_PIDR2);
    spi_num = ((VM_GLOBAL_VIRQ_NR + 32) >> 5) - 1;
	tmp_typer = (vm->vcpu_num << 5) | (9 << 19) | spi_num;
	vgic_sysreg_write32(tmp_typer, gicd->gicd_regs_base, VGICD_TYPER);
	/* Init spinlock */
	ZVM_SPINLOCK_INIT(&gicd->gicd_lock);

    for (i = 0; i < VGIC_RDIST_SIZE/VGIC_RD_SGI_SIZE + 1; i++) {
        gicr = (struct virt_gic_gicr *)k_malloc(sizeof(struct virt_gic_gicr));
        if(!gicr){
			return -EMMAO;
		}
		/* store the vcpu id for gicr */
        gicr->vcpu_id = i;
		/* init redistribute size */
		gicr->gicr_rd_size = VGIC_RD_BASE_SIZE;
		gicr->gicr_rd_reg_base = k_malloc(gicr->gicr_rd_size);
		if(!gicr->gicr_rd_reg_base){
			ZVM_LOG_ERR("Allocat memory for gicr_rd error! \n");
        	return -EMMAO;
		}
		memset(gicr->gicr_rd_reg_base, 0, gicr->gicr_rd_size);
		/* init sgi redistribute size */
		gicr->gicr_sgi_size = VGIC_SGI_BASE_SIZE;
		gicr->gicr_sgi_reg_base = k_malloc(gicr->gicr_sgi_size);
		if(!gicr->gicr_sgi_reg_base){
			ZVM_LOG_ERR("Allocat memory for gicr_sgi error! \n");
        	return -EMMAO;
		}
		memset(gicr->gicr_sgi_reg_base, 0, gicr->gicr_sgi_size);

		gicr->gicr_rd_base = gicr_base + VGIC_RD_SGI_SIZE*i;
		gicr->gicr_sgi_base = gicr->gicr_rd_base + VGIC_RD_BASE_SIZE;
		vgic_sysreg_write32(0x3<<4, gicr->gicr_rd_reg_base, VGICR_PIDR2);
		/* Init spinlock */
		ZVM_SPINLOCK_INIT(&gicr->gicr_lock);

		if(i >= vm->vcpu_num - 1){
			/* set last gicr region flag here, means it is the last gicr region */
			vgic_sysreg_write64(GICR_TYPER_LAST_FLAG, gicr->gicr_rd_reg_base, VGICR_TYPER);
			vgic_sysreg_write64(GICR_TYPER_LAST_FLAG, gicr->gicr_sgi_reg_base, VGICR_TYPER);
		}else{
			vgic_sysreg_write64((uint64_t)i << 32, gicr->gicr_rd_reg_base, VGICR_TYPER);
			vgic_sysreg_write64((uint64_t)i << 32, gicr->gicr_sgi_reg_base, VGICR_TYPER);
		}

		gicv3_vdev->gicr[i] = gicr;
    }

	ZVM_LOG_INFO("** List register num: %lld \n", VGIC_TYPER_LR_NUM);
	vgicv3_lrs_init();

	return 0;
}

/**
 * @brief init vm gic device for each vm. Including:
 * 1. creating virt device for vm.
 * 2. building memory map for this device.
*/
static int vm_vgicv3_init(const struct device *dev, struct vm *vm, struct virt_dev *vdev_desc)
{
	int ret;
	uint32_t gicd_base, gicd_size, gicr_base, gicr_size;
	struct virt_dev *virt_dev;
	struct vgicv3_dev *vgicv3;

    gicd_base = DEV_VGICV3(dev)->gicd_base;
    gicd_size = DEV_VGICV3(dev)->gicd_size;
    gicr_base = DEV_VGICV3(dev)->gicr_base;
    gicr_size = DEV_VGICV3(dev)->gicr_size;;
	/* check gic device */
	if(!gicd_base || !gicd_size  || !gicr_base  || !gicr_size){
        ZVM_LOG_ERR("GIC device has init error!");
        return -ENODEV;
	}

	/* Init virtual device for vm. */
	virt_dev = vm_virt_dev_add(vm, dev->name, false, false, gicd_base,
						gicd_base, gicr_base+gicr_size-gicd_base, 0, 0);
	if(!virt_dev){
        return -ENODEV;
	}

	/* Init virtual gic device for virtual device. */
	vgicv3 = (struct vgicv3_dev *)k_malloc(sizeof(struct vgicv3_dev));
    if (!vgicv3) {
        ZVM_LOG_ERR("Allocat memory for vgicv3 error \n");
        return -ENODEV;
    }
    ret = vdev_gicv3_init(vm, vgicv3, gicd_base, gicd_size, gicr_base, gicr_size);
    if(ret){
        ZVM_LOG_ERR("Init virt gicv3 error \n");
        return -ENODEV;
    }

	/* get the private data for vgicv3 */
	virt_dev->priv_data = vgicv3;
	virt_dev->priv_vdev = (void *)dev;

	return 0;
}

/**
 * @brief The init function of vgic, it provides the
 * gic hardware device information to ZVM.
*/
static int vgicv3_init(const struct device *dev)
{
	return 0;
}

static struct gicv3_vdevice vgicv3_cfg_port = {
	.gicd_base = VGIC_DIST_BASE,
	.gicd_size = VGIC_DIST_SIZE,
	.gicr_base = VGIC_RDIST_BASE,
	.gicr_size = VGIC_RDIST_SIZE,
};

static struct virt_device_config virt_gicv3_cfg = {
	.device_config = &vgicv3_cfg_port,
};

static struct virt_device_data virt_gicv3_data_port = {
	.vdevice_type = 0,
};

/**
 * @brief vgic device operations api.
*/
static const struct virt_device_api virt_gicv3_api = {
	.init_fn = vm_vgicv3_init,
	.virt_device_read = vgic_vdev_mem_read,
	.virt_device_write = vgic_vdev_mem_write,
};

/**
 * @brief Define the vgic description for zvm.
*/
DEVICE_DT_DEFINE(DT_ALIAS(vmvgic),
            &vgicv3_init,
            NULL,
            &virt_gicv3_data_port,
            &virt_gicv3_cfg, POST_KERNEL,
            CONFIG_VM_VGICV3_INIT_PRIORITY,
            &virt_gicv3_api);
