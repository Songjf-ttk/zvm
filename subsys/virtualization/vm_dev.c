/*
 * Copyright 2021-2022 HNU-ESNL
 * Copyright 2023 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <sys/dlist.h>
#include <drivers/uart.h>
#include <virtualization/zvm.h>
#include <virtualization/vm_dev.h>
#include <virtualization/vm_mm.h>
#include <virtualization/vdev/vgic_v3.h>
#include <virtualization/vm_console.h>
#include <virtualization/vdev/fiq_debugger.h>
#include <virtualization/vdev/virt_device.h>
#include <virtualization/vdev/virtio/virtio.h>
#include <virtualization/vdev/virtio/virtio_blk.h>
#include <virtualization/vdev/virtio/virtio_mmio.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

int __weak vm_init_bdspecific_device(struct vm *vm)
{
    return 0;
}

static int vm_vdev_mem_add(struct vm *vm, struct virt_dev *vdev)
{
    uint32_t attrs = 0;

    /*If device is emulated, set access off attrs*/
    if (vdev->dev_pt_flag && !vdev->shareable) {
        attrs = MT_VM_DEVICE_MEM;
    }else{
        attrs = MT_VM_DEVICE_MEM | MT_S2_ACCESS_OFF;
    }

    return vm_vdev_mem_create(vm->vmem_domain, vdev->vm_vdev_paddr,
            vdev->vm_vdev_vaddr, vdev->vm_vdev_size, attrs);

}

struct virt_dev *vm_virt_dev_add(struct vm *vm, const char *dev_name, bool pt_flag,
                bool shareable, uint32_t dev_pbase, uint32_t dev_vbase,
                    uint32_t dev_size, uint32_t dev_hirq, uint32_t dev_virq)
{
    uint16_t name_len;
    int ret;
    struct virt_dev *vm_dev;

    vm_dev = (struct virt_dev *)k_malloc(sizeof(struct virt_dev));
	if (!vm_dev) {
        ZVM_LOG_ERR("Allocate memory for vm device error!\n");
        return NULL;
    }

    name_len = strlen(dev_name);
    name_len = name_len > VIRT_DEV_NAME_LENGTH ? VIRT_DEV_NAME_LENGTH : name_len;
    strncpy(vm_dev->name, dev_name, name_len);
    vm_dev->name[name_len] = '\0';

    vm_dev->dev_pt_flag = pt_flag;
    vm_dev->shareable = shareable;
    vm_dev->vm_vdev_paddr = dev_pbase;
    vm_dev->vm_vdev_vaddr = dev_vbase;
    vm_dev->vm_vdev_size = dev_size;

    ret = vm_vdev_mem_add(vm, vm_dev);
    if(ret){
        ZVM_LOG_ERR("Map vm device memory error!\n");
        return NULL;
    }
    vm_dev->virq = dev_virq;
    vm_dev->hirq = dev_hirq;
    vm_dev->vm = vm;

    /*Init private data and vdev*/
    vm_dev->priv_data = NULL;
    vm_dev->priv_vdev = NULL;

    sys_dlist_append(&vm->vdev_list, &vm_dev->vdev_node);

    return vm_dev;
}

#define DEV_DATA(dev) \
	((struct virt_device_data *)(dev)->data)

int vdev_mmio_abort(arch_commom_regs_t *regs, int write, uint64_t addr,
                uint64_t *value, uint16_t size)
{
    uint64_t *reg_value = value;
    struct vm *vm;
    struct virt_dev *vdev;
    struct  _dnode *d_node, *ds_node;
    struct virtual_device_instance *vdevice_instance;
    const struct device *dev;

    vm = get_current_vm();

    SYS_DLIST_FOR_EACH_NODE_SAFE(&vm->vdev_list, d_node, ds_node){
        vdev = CONTAINER_OF(d_node, struct virt_dev, vdev_node);
        vdevice_instance = (struct virtual_device_instance *)vdev->priv_data;

        if (vdev->shareable) {
			if ((addr >= vdev->vm_vdev_vaddr) && (addr < vdev->vm_vdev_vaddr + vdev->vm_vdev_size)) {
                dev = (const struct device* const)vdev->priv_data;
				if (write) {
					return ((struct virtio_mmio_driver_api *)((struct virt_device_api \
                    *)dev->api)->device_driver_api)->write(vdev, addr - vdev->vm_vdev_vaddr, 0, (uint32_t)*reg_value, size);
				} else {
					return ((struct virtio_mmio_driver_api *)((struct virt_device_api \
                    *)dev->api)->device_driver_api)->read(vdev, addr - vdev->vm_vdev_vaddr, (uint32_t *)reg_value, size);
				}
			}
		} else if (!strcmp(vdev->name, "VM_VGIC")) {
            if ((addr >= vdev->vm_vdev_paddr) && (addr < vdev->vm_vdev_paddr + vdev->vm_vdev_size)) {
                dev = (const struct device* const)vdev->priv_vdev;
                if (write) {
                    return ((const struct virt_device_api * \
                        const)(dev->api))->virt_device_write(vdev, addr, reg_value);
                }else{
                    return ((const struct virt_device_api * \
                        const)(dev->api))->virt_device_read(vdev, addr, reg_value);
                }
            }
        } else if(vdevice_instance != NULL){
            if(DEV_DATA(vdevice_instance)->vdevice_type & VM_DEVICE_PRE_KERNEL_1){
                if ((addr >= vdev->vm_vdev_paddr) && (addr < vdev->vm_vdev_paddr + vdev->vm_vdev_size)) {
                    if (write) {
                        return ((const struct virt_device_api * \
                            const)(vdevice_instance->api))->virt_device_write(vdev, addr, reg_value);
                    }else{
                        return ((const struct virt_device_api * \
                            const)(vdevice_instance->api))->virt_device_read(vdev, addr, reg_value);
                    }
                }
            }
        }
    }

    /* Not found the vdev */
    ZVM_LOG_WARN("There are no virtual dev for this addr, addr : 0x%llx \n", addr);
    return -ENODEV;
}

int vm_unmap_ptdev(struct virt_dev *vdev, uint64_t vm_dev_base,
         uint64_t vm_dev_size, struct vm *vm)
{
    int ret = 0;
    ARG_UNUSED(ret);
    uint64_t p_base, v_base, p_size, v_size;

    p_base = vdev->vm_vdev_paddr;
    p_size = vdev->vm_vdev_size;
    v_base = vm_dev_base;
    v_size = vm_dev_size;

    if (p_size != v_size || p_size == 0) {
        ZVM_LOG_WARN("The device is not matching, can not allocat this dev to the vm!");
        return -ENODEV;
    }

    return arch_vm_dev_domain_unmap(p_size, v_base, v_size, vdev->name, vm);

}

int vm_vdev_pause(struct vcpu *vcpu)
{
    ARG_UNUSED(vcpu);
    return 0;
}

int handle_vm_device_emulate(struct vm *vm, uint64_t pa_addr)
{
    bool chosen_flag = false;
    int ret;
    struct virt_dev *vm_dev, *chosen_dev = NULL;
    struct zvm_dev_lists *vdev_list;
    struct  _dnode *d_node, *ds_node;
    const struct device *dev;

    vdev_list = get_zvm_dev_lists();
    SYS_DLIST_FOR_EACH_NODE_SAFE(&vdev_list->dev_idle_list, d_node, ds_node){
        vm_dev = CONTAINER_OF(d_node, struct virt_dev, vdev_node);
        /* Match the memory address ? */
        if(pa_addr >= vm_dev->vm_vdev_paddr && pa_addr < (vm_dev->vm_vdev_paddr+vm_dev->vm_vdev_size)){
            chosen_flag = true;
            break;
        }
    }

    if(chosen_flag){
        chosen_dev = vm_virt_dev_add(vm, vm_dev->name, vm_dev->dev_pt_flag, vm_dev->shareable,
                            vm_dev->vm_vdev_paddr, vm_dev->vm_vdev_paddr,
                            vm_dev->vm_vdev_size, vm_dev->hirq, vm_dev->hirq);
        if(!chosen_dev){
            ZVM_LOG_WARN("there are no idle device %s for vm!", vm_dev->name);
            return -ENODEV;
        }
        /* move device to used node! */
        sys_dlist_remove(&vm_dev->vdev_node);
        sys_dlist_append(&vdev_list->dev_used_list, &vm_dev->vdev_node);

        vm_device_irq_init(vm, chosen_dev);
        dev = (struct device *)vm_dev->priv_data;
        chosen_dev->priv_data = (void *)dev;
        vdev_irq_callback_user_data_set(dev, vm_device_callback_func, chosen_dev);

        ret = ((struct virt_device_api *)dev->api)->init_fn(dev, vm, chosen_dev);
        if (ret) {
            ZVM_LOG_WARN(" Init device error! \n");
            return -EFAULT;
        }

        return 0;
    }

    return -ENODEV;
}

static void zvm_virtio_emu_register(void)
{
#ifdef CONFIG_VM_VIRTIO_BLOCK
    virtio_register_emulator(&virtio_blk);
#endif
}

int vm_device_init(struct vm *vm)
{
    int ret, i;

    sys_dlist_init(&vm->vdev_list);

    ret = vm_intctrl_vdev_create(vm);
    if (ret) {
        ZVM_LOG_WARN(" Init interrupt controller device error! \n");
		return -ENODEV;
    }

    ret = vm_console_create(vm);
	if (ret) {
        ZVM_LOG_WARN("Init vm debug console error! \n");
        return -EMMAO;
    }

	/* Assign ids to virtual devices. */
	for (i = 0; i < zvm_virtual_devices_count_get(); i++) {
		const struct virtual_device_instance *virtual_device = zvm_virtual_device_get(i);
        ZVM_LOG_INFO("Device name: %s. \n", virtual_device->name);
        /*If the virtual device is nessenary for vm*/
        if(virtual_device->data->vdevice_type & VM_DEVICE_PRE_KERNEL_1){
            virtual_device->api->init_fn(NULL, vm, NULL);
        }
	}

#ifdef CONFIG_VM_VIRTIO_MMIO
	virtio_dev_list_init();
    virtio_drv_list_init();
    zvm_virtio_emu_register();
#endif
    /* @TODO: scan the dtb and get the device's node. */
    /* Board specific device init, for example fig debugger. */
    switch (vm->os->type){
    case OS_TYPE_LINUX:
        ret = vm_init_bdspecific_device(vm);
        break;
    default:
        break;
    }

    /* @TODO: scan the dtb and get the device's node. */
    return ret;
}
