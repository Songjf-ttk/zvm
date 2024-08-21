/*
 * Copyright 2024 HNU-ESNL
 * Copyright 2024 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <kernel.h>
#include <zephyr.h>
#include <string.h>
#include <device.h>
#include <devicetree.h>
#include <kernel_structs.h>
#include <init.h>
#include <drivers/pcie/pcie_ecam.h>
#include <virtualization/zvm.h>
#include <virtualization/vm.h>
#include <virtualization/os/os.h>
#include <virtualization/vdev/virt_pci.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define VM_PCI_NAME			vm_pci_bus

#define PCIE_BUS_DEVICE			DEVICE_DT_GET(DT_ALIAS(vmpcie))
#define PCIE_ECAM_CONFIG(dev)	\
		((struct pcie_ctrl_config *)(dev)->config)
#define PCIE_ECAM_DATA(dev)		\
		((struct pcie_ecam_data *)(dev)->data)

#define DEV_DATA(dev) \
	((struct virt_device_data *)(dev)->data)
#define DEV_CFG(dev) \
	((struct virt_device_config *)(dev)->cfg)

#define PCI_CFG(dev) \
	((struct virt_pci_bus_config *)(DEV_CFG(dev))->device_config)
#define PCI_DATA(dev) \
	((struct virt_pci_bus_data *)(DEV_DATA(dev))->device_data)

#define DEFINE_VPCI_DEVICE(node_id) { .type = DT_PROP(node_id, type), \
							.domain = DT_PROP(node_id, domain), \
							.bdf = DT_PROP(node_id, bdf), \
							.bar_mask[0] = DT_PROP(node_id, bar_mask0), \
							.bar_mask[1] = DT_PROP(node_id, bar_mask1), \
							.bar_mask[2] = DT_PROP(node_id, bar_mask2), \
							.bar_mask[3] = DT_PROP(node_id, bar_mask3), \
							.bar_mask[4] = DT_PROP(node_id, bar_mask4), \
							.bar_mask[5] = DT_PROP(node_id, bar_mask5), \
							.shmem_region_start = DT_PROP(node_id, shmem_region_start), \
							.shmem_dev_id = DT_PROP(node_id, shmem_dev_id), \
							.shmem_peers = DT_PROP(node_id, shmem_peers), \
							.shmem_protocol = DT_PROP(node_id, shmem_protocol), \
							},

static const struct virtual_device_instance *pci_virtual_device_instance;

static struct virt_pci_device pci_devices[] = {
	DT_FOREACH_STATUS_OKAY(virt_pci, DEFINE_VPCI_DEVICE)
};

int virt_pci_emulation(struct vm *vm, uint64_t pdev_addr, int write, uint64_t *value)
{
	int i;
	uint64_t wvalue, rvalue;
	uint64_t cfg_base, cfg_size, hdev_base;
	uint64_t range_base, range_size;
	struct virt_dev *vdev;

	wvalue = *(uint64_t *)value;
	cfg_base = DEV_CFG(pci_virtual_device_instance)->reg_base;
	cfg_size = DEV_CFG(pci_virtual_device_instance)->reg_size;
	hdev_base = PCI_DATA(pci_virtual_device_instance)->pci_hva_base;

	/*pci bus configuration space?*/
	if(pdev_addr >= cfg_base  && pdev_addr < cfg_base+cfg_size){
		if(write) {
			sys_write64(wvalue, pdev_addr-cfg_base + hdev_base);
		}else {
			rvalue = sys_read64(pdev_addr-cfg_base + hdev_base);
			*(uint64_t *)value = rvalue;
		}
		return 0;
	}

	/*pci device space?*/
	for(i=0; i<PCIE_REGION_MAX; i++){
		range_base = PCIE_ECAM_DATA(PCIE_BUS_DEVICE)->regions[i].phys_start;
		range_size = PCIE_ECAM_DATA(PCIE_BUS_DEVICE)->regions[i].size;
		if(pdev_addr >= range_base && pdev_addr < range_base+range_size){
			if(write) {
				sys_write64(wvalue, pdev_addr);
			}else {
				rvalue = sys_read64(pdev_addr);
				*(uint64_t *)value = rvalue;
			}
			
			return 0;
			/*@TODO:pci passthrough interrupt is not consider now.*/
		}
	}

	return -ENODEV;
}

/**
 * @brief: Get the virtual virtual pci device.
*/
static int virt_pci_bus_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	int i;

	for (i = 0; i < zvm_virtual_devices_count_get(); i++) {
		const struct virtual_device_instance *virtual_device = zvm_virtual_device_get(i);
		if(strcmp(virtual_device->name, TOSTRING(VM_PCI_NAME))){
			continue;
		}
		/*Get device info from pcie physical device.*/
		DEV_CFG(virtual_device)->reg_base = PCIE_ECAM_DATA(PCIE_BUS_DEVICE)->cfg_phys_addr;
		DEV_CFG(virtual_device)->reg_size = PCIE_ECAM_DATA(PCIE_BUS_DEVICE)->cfg_size;
		PCI_DATA(virtual_device)->pci_hva_base = PCIE_ECAM_DATA(PCIE_BUS_DEVICE)->cfg_addr;
		printk("cfg_phys_addr: %llx \n", PCIE_ECAM_DATA(PCIE_BUS_DEVICE)->cfg_phys_addr);
		printk("cfg_size: %llx \n", PCIE_ECAM_DATA(PCIE_BUS_DEVICE)->cfg_size);
		printk("reg_base: %llx \n", DEV_CFG(virtual_device)->reg_base);

		if(!DEV_CFG(virtual_device)->reg_base && !PCI_CFG(virtual_device)->pci_is_virtual){
			ZVM_LOG_ERR("Do not support hardware pci device now.\n");
			return -ENODEV;
		}

		DEV_DATA(virtual_device)->vdevice_type |= VM_DEVICE_PRE_KERNEL_1;
		pci_virtual_device_instance = virtual_device;
		break;
	}
	return 0;
}

static struct virt_pci_bus_config pci_bus_priv_cfg = {
	.pci_is_virtual = true,
	.pci_domain = 1,
	.pci_config_end_bus = 0,
};

static struct virt_device_config virt_pci_bus_cfg = {

	/*TODO: Irq is unknown.*/
	.hirq_num = VM_DEVICE_INVALID_VIRQ,

	.device_config = &pci_bus_priv_cfg,
};

static struct virt_pci_bus_data pci_privdate;

static struct virt_device_data virt_pci_bus_data_port = {
	.device_data = &pci_privdate,
};

/**
 * @brief init vm clock syscon device for each vm.
*/
static int vm_pci_bus_init(const struct device *dev, struct vm *vm, struct virt_dev *vdev_desc)
{
	ARG_UNUSED(dev);
	int i;
	struct virt_dev *vm_dev;
	struct virt_pci_device *pci_device;

	/* Is there any pci device exist? */
	switch (vm->os->type) {
	case OS_TYPE_LINUX:
#if	DT_NODE_EXISTS(DT_ALIAS(vmivshmem0))
		break;
#endif
		return 0;
		break;
	case OS_TYPE_ZEPHYR:
#if	DT_NODE_EXISTS(DT_ALIAS(vmivshmem1))
		break;
#endif
		return 0;
	default:
		return 0;
	}

	vm_dev = vm_virt_dev_add_no_memmap(vm, pci_virtual_device_instance->name, false, false,
						DEV_CFG(pci_virtual_device_instance)->reg_base, PCI_DATA(pci_virtual_device_instance)->pci_hva_base,
						DEV_CFG(pci_virtual_device_instance)->reg_size, VM_DEVICE_INVALID_VIRQ,
						VM_DEVICE_INVALID_VIRQ);
	if(!vm_dev){
		return -ENODEV;
	}

	vm_dev->priv_data = pci_virtual_device_instance;

	return 0;
}

int pci_bus_vdev_mem_read(struct virt_dev *vdev, uint64_t addr, uint64_t *value)
{
	uint32_t read_value;
	read_value = sys_read32(addr);
	*(uint32_t *)value = read_value;

	printk("Device-%s Read:addr is %llx, value is %x\n", vdev->name, addr, read_value);

	return 0;
}

int pci_bus_vdev_mem_write(struct virt_dev *vdev, uint64_t addr, uint64_t *value)
{
	uint32_t be_write_value, write_value, af_write_value;

	write_value = *(uint32_t *)value;
	be_write_value = sys_read32(addr);
	sys_write32(write_value, addr);
	af_write_value = sys_read32(addr);

	printk("Device-%s Write:addr is %llx, be_value is %x, ne_value is %x, af_value is %x\n",
			 vdev->name, addr, be_write_value, write_value, af_write_value);

	return 0;
}

static const struct virt_device_api virt_pci_bus_api = {
	.init_fn = vm_pci_bus_init,
	.virt_device_read = pci_bus_vdev_mem_read,
	.virt_device_write = pci_bus_vdev_mem_write,
};

/* Init virtual pci device. */
ZVM_VIRTUAL_DEVICE_DEFINE(virt_pci_bus_init,
			POST_KERNEL, CONFIG_VM_PCI_BUS_INIT_PRIORITY,
			VM_PCI_NAME,
			virt_pci_bus_data_port,
			virt_pci_bus_cfg,
			virt_pci_bus_api);