/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <arch/cpu.h>
#include <arch/arm64/lib_helpers.h>
#include <sys/printk.h>
#include <init.h>
#include <logging/log.h>
#include <virtualization/zvm.h>
#include <virtualization/os/os.h>
#include <virtualization/os/os_zephyr.h>
#include <virtualization/os/os_linux.h>
#include <virtualization/vm.h>
#include <virtualization/vm_dev.h>
#include <virtualization/vdev/virt_device.h>

LOG_MODULE_REGISTER(ZVM_MODULE_NAME);

extern const struct device __device_start[];
extern const struct device __device_end[];

struct zvm_manage_info *zvm_overall_info;       /*@TODO,This may need to replace by macro later*/
static struct zvm_dev_lists  zvm_overall_dev_lists;

/**
 * Template of zephyr and linux os.
*/
z_vm_info_t z_overall_vm_infos[] = {
    {
        .entry_point = ZEPHYR_VMSYS_BASE,
        .vm_virt_base = ZEPHYR_VMSYS_BASE,
        .vm_sys_size = ZEPHYR_VMSYS_SIZE,
        .vm_image_base = ZEPHYR_VM_IMAGE_BASE,
        .vm_image_size = ZEPHYR_VM_IMAGE_SIZE,
        .vcpu_num = VM_DEFAULT_VCPU_NUM,
        .vm_os_type = OS_TYPE_ZEPHYR,
    },
    {
        .entry_point = LINUX_VMSYS_BASE,
        .vm_virt_base = LINUX_VMSYS_BASE,
        .vm_sys_size = LINUX_VMSYS_SIZE,
        .vm_image_base = LINUX_VM_IMAGE_BASE,
        .vm_image_size = LINUX_VM_IMAGE_SIZE,
        .vcpu_num = VM_DEFAULT_VCPU_NUM,
        .vm_os_type = OS_TYPE_LINUX,
    },

};

/**
 * @brief zvm_hwsys_info_init aim to init zvm_info for the hypervisor
 * Two stage for this function:
 * 1. Init zvm_overall for some of para in struct
 * 2. get hareware information form dts
 * TODO: Add hardware here.
 */
static int zvm_hwsys_info_init(struct zvm_hwsys_info *z_info)
{
    int cpu_ret = -1, mem_ret = -1;
    ARG_UNUSED(cpu_ret);
    ARG_UNUSED(mem_ret);

    z_info->phy_mem_used = 0;
/** cpu_ret = dt_get_cpu_info(z_info);
    mem_ret = dt_get_mem_size(z_info);*/

    return 0;
}

/**
 * @brief ipi handler for zvm.
 *
 */
void zvm_ipi_handler(void)
{
    struct vcpu *vcpu = _current_vcpu;

    /* judge whether it is a vcpu thread */
    if (vcpu) {
        vm_ipi_handler(vcpu->vm);
    }

}

/**
 * @brief load os image from stroage address here
 *
 * @param os_type
 * @return int
 */
int load_os_image(struct vm *vm)
{
    int ret = 0;

    switch (vm->os->type){
    case OS_TYPE_LINUX:
        ret = load_linux_image(vm->vmem_domain);
        if (ret) {
            ZVM_LOG_WARN("Load linux error ");
        }
        break;
    case OS_TYPE_ZEPHYR:
        ret = load_zephyr_image(vm->vmem_domain);
        if (ret) {
            ZVM_LOG_WARN("Load zephyr error ");
        }
        break;
    default:
        ZVM_LOG_WARN("Unsupport OS image!");
/*		load_other_image();*/
        break;
	}
    return ret;
}


void zvm_info_print(struct zvm_hwsys_info *z_info){
    printk(">---- System's information ----<\n");
    printk("  All phy-cpu: %d\n", z_info->phy_cpu_num);
    printk("  CPU's type : %s\n", z_info->cpu_type);
    printk("  All phy-mem: %.2fMB\n", (float)(z_info->phy_mem)/DT_MB);
    printk("  Memory used: %.2fMB\n", (float)(z_info->phy_mem_used)/DT_MB);
    printk(">------------------------------<\n");
}

/**
 * @brief This function aim to init zvm dev on zvm init stage
 * @TODO: may add later
 * @return int
 */
static int zvm_dev_ops_init()
{
    return 0;
}

/**
 * @brief Init zvm overall device here
 * Two stage For this function:
 * 1. Create and init zvm_over_all struct
 * 2. Pass information from
 * @return int : the error code there
 */
static int zvm_overall_init(void)
{
    int ret = 0;

    /* First initialize zvm_overall_info->hw_info. */
    zvm_overall_info = (struct zvm_manage_info*)k_malloc  \
                            (sizeof(struct zvm_manage_info));
    if (!zvm_overall_info) {
        return -ENOMEM;
    }

    zvm_overall_info->hw_info = (struct zvm_hwsys_info *)
            k_malloc(sizeof(struct zvm_hwsys_info));
    if (!zvm_overall_info->hw_info) {
        return -ENOMEM;
    }
    memset(zvm_overall_info, 0, (sizeof(struct zvm_manage_info)));
    zvm_overall_info->hw_info = (struct zvm_hwsys_info*)
            k_malloc(sizeof(struct zvm_hwsys_info));
    if (!zvm_overall_info->hw_info) {
        ZVM_LOG_ERR("Allocate memory for zvm_overall_info Error.\n");
        /*
         * Too cumbersome resource release way.
         * We can use resource stack way to manage these resouce.
         */
        k_free(zvm_overall_info->hw_info);
        k_free(zvm_overall_info);
        return -ENOMEM;
    }
    ZVM_SPINLOCK_INIT(&zvm_overall_info->spin_zmi);

    ret = zvm_hwsys_info_init(zvm_overall_info->hw_info);
    if (ret) {
        k_free(zvm_overall_info->hw_info);
        k_free(zvm_overall_info);
        return ret;
    }
    /* Then initialize the last value in zvm_overall_info. */
    zvm_overall_info->alloced_vmid = 0;
    zvm_overall_info->vm_total_num = 0;

    return ret;
}

/**
 * @brief Add all the passthrough device to the zvm_overall_list.
*/
int zvm_init_idle_device(const struct device *dev, struct virt_dev *vdev,
                            struct zvm_dev_lists *dev_list)
{
    uint16_t name_len;
    struct virt_dev *vm_dev = vdev;
    
    /*TODO：Determine whether to connect directly based on device type*/
    vm_dev->dev_pt_flag = true;

    if(strcmp(((struct virt_device_config *)dev->config)->device_type, "virtio") == 0) {
        vm_dev->shareable = true;
    } else {
        vm_dev->shareable = false;
    }

    name_len = strlen(dev->name);
    name_len = name_len > VIRT_DEV_NAME_LENGTH ? VIRT_DEV_NAME_LENGTH : name_len;
    strncpy(vm_dev->name, dev->name, name_len);
    vm_dev->name[name_len] = '\0';

    vm_dev->vm_vdev_paddr = ((struct virt_device_config *)dev->config)->reg_base;
    vm_dev->vm_vdev_vaddr = VM_DEVICE_INVALID_BASE;
    vm_dev->vm_vdev_size = ((struct virt_device_config *)dev->config)->reg_size;
    vm_dev->hirq = ((struct virt_device_config *)dev->config)->hirq_num;
    vm_dev->virq = VM_DEVICE_INVALID_VIRQ;
    vm_dev->vm = NULL;

    vm_dev->priv_data = (void *)dev;

    printk("Init passthrough device %s successful! \n", vm_dev->name);

    sys_dnode_init(&vm_dev->vdev_node);
    sys_dlist_append(&dev_list->dev_idle_list, &vm_dev->vdev_node);

    return 0;
}

/**
 * @brief Provide physical dev info to zvm system:
 * 1. Find all available devices information on the board;
 * 2. Build a idle dev list for the system;
 * 3. VM init function can get dev info from it.
 * @return int
 */
static int zvm_devices_list_init(void)
{
    struct virt_dev *vm_dev;
    const struct device *dev;

    sys_dlist_init(&zvm_overall_dev_lists.dev_idle_list);
    sys_dlist_init(&zvm_overall_dev_lists.dev_used_list);

    /* scan the host dts and get the device list */
	for (dev = __device_start; dev != __device_end; dev++) {
        /**
         * through the `init_res` to judge whether the device is
         *  ready to allocate to vm.
         */
		if (dev->state->init_res == VM_DEVICE_INIT_RES) {
            vm_dev = (struct virt_dev*)k_malloc(sizeof(struct virt_dev));
            if (vm_dev == NULL) {
                return -ENOMEM;
            }
            zvm_init_idle_device(dev, vm_dev, &zvm_overall_dev_lists);
		}
	}

    return 0;
}

/**
 * @brief Get the zvm dev lists object
 * @return struct zvm_dev_lists*
 */
struct zvm_dev_lists* get_zvm_dev_lists(void)
{
    return &zvm_overall_dev_lists;
}

/*
 * @brief Main work of this function is to initialize zvm module.
 *
 * All works include:
 * 1. Checkout hardware support for  hypervisor；
 * 2. Initialize struct variable "zvm_overall_info";
 * 3. @TODO: Init zvm dev and opration function.
 */
static int zvm_init(const struct device *dev)
{
    ARG_UNUSED(dev);
    int ret = 0;
    void *op = NULL;

    ret = zvm_arch_init(op);
    if (ret) {
       ZVM_LOG_ERR("zvm_arch_init failed here ! \n");
       return ret;
    }

    ret = zvm_overall_init();
    if (ret) {
        ZVM_LOG_ERR("Init zvm_overall struct error. \n ZVM init failed ! \n");
        return ret;
    }

    ret = zvm_devices_list_init();
    if (ret) {
        ZVM_LOG_ERR("Init zvm_dev_list struct error. \n ZVM init failed ! \n");
        return ret;
    }

    /*TODO: ready to init zvm_dev and it's ops */
    zvm_dev_ops_init();

    return ret;
}

/* To simple init zvm here, @TODO instan it as a device */
SYS_INIT(zvm_init, POST_KERNEL, CONFIG_ZVM_INIT_PRIORITY);
