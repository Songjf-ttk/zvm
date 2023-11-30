/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <ksched.h>
#include <sys/arch_interface.h>

#include <virtualization/os/os.h>
#include <virtualization/os/os_zephyr.h>
#include <virtualization/os/os_linux.h>
#include <virtualization/zvm.h>
#include <virtualization/vdev/vgic_v3.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

/* Structure for parsing args. */
struct getopt_state *state = NULL;


int zvm_new_guest(size_t argc, char **argv)
{
	int ret;
	struct vm *new_vm = NULL;
	struct z_vm_info *vm_info = NULL;

	if (is_vmid_full()) {
		ZVM_LOG_WARN("System vm's num has reached the limit.\n");
		return -ENXIO;
	}

	ret = vm_sysinfo_init(argc, argv, &new_vm, state, &vm_info);
	if (ret) {
		return ret;
	}

	ret = vm_create(vm_info, new_vm);
	if (ret) {
		k_free(new_vm);
        k_free(vm_info);
		ZVM_LOG_WARN("Can not create vm struct, VM struct init failed!\n");
		return ret;
	}
	ZVM_LOG_INFO("\n Create VM instance successful! \n");

	ret = vm_ops_init(new_vm);
	if (ret) {
		ZVM_LOG_WARN("VM ops init failed!\n");
		return ret;
	}
	ZVM_LOG_INFO("\n Init VM ops successful! \n");

	ret = vm_irq_block_init(new_vm);
	if (ret < 0) {
        ZVM_LOG_WARN(" Init vm's irq block error!\n");
        return ret;
    }
	ZVM_LOG_INFO("\n Init VM irq block successful! \n");

	ret = vm_vcpus_init(new_vm);
	if (ret < 0) {
		ZVM_LOG_WARN("create vcpu error! \n");
		return -ENXIO;
	}
	ZVM_LOG_INFO("\n Init VM vcpus instances successful! \n");

	ret = vm_device_init(new_vm);
	if (ret) {
		ZVM_LOG_WARN(" Init vm's virtual device error! \n");
		return ret;
	}
	ZVM_LOG_INFO("\n Init VM devices successful! \n");

   	ret = vm_mem_init(new_vm);
	if(ret < 0){
		return ret;
	}
	ZVM_LOG_INFO("\n Init VM memory successful! \n");

	k_free(vm_info);

	ZVM_PRINTK("\n|*********************************************|\n");
	ZVM_PRINTK("|******\t Create vm successful!  **************| \n");
	ZVM_PRINTK("|******\t\t VM INFO \t \t******| \n");
	ZVM_PRINTK("|******\t VM-NAME:     %s \t******| \n", new_vm->vm_name);
	ZVM_PRINTK("|******\t VM-ID: \t %d \t\t******| \n", new_vm->vmid);
	ZVM_PRINTK("|******\t VCPU NUM: \t %d \t\t******| \n", new_vm->vcpu_num);

	switch (new_vm->os->type) {
	case OS_TYPE_LINUX:
		ZVM_PRINTK("|******\t VMEM SIZE: \t %d(M) \t******| \n",
		LINUX_VMSYS_SIZE/(1024*1024));
		break;
	case OS_TYPE_ZEPHYR:
		ZVM_PRINTK("|******\t VMEM SIZE:  \t %d(M) \t\t******| \n",
		ZEPHYR_VMSYS_SIZE/(1024*1024));
		break;
	default:
		ZVM_PRINTK("|******\t OTHER VM, NO MEMORY MSG \t\t******| \n");
	}
	ZVM_PRINTK("|*********************************************|\n");

	return 0;
}


int zvm_run_guest(size_t argc, char **argv)
{
	uint16_t vm_id;
	int ret = 0;
	struct vm *vm;

	ZVM_LOG_INFO("\n Ready to run VM. \n");
	vm_id = z_parse_run_vm_args(argc, argv, state);
	if (!(BIT(vm_id) & zvm_overall_info->alloced_vmid)) {
        ZVM_LOG_WARN("This vmid is not exist!\n Please input zvm info to show info! \n");
		return -EINVAL;
    }

	vm = zvm_overall_info->vms[vm_id];
	if (vm->vm_status & VM_STATE_RUNNING) {
		ZVM_LOG_WARN("This vm is already running! \n Please input zvm info to check vms! \n");
		return -EINVAL;
	}

	if (vm->vm_status & (VM_STATE_NEVER_RUN | VM_STATE_PAUSE)) {
		if (vm->vm_status & VM_STATE_NEVER_RUN) {
			load_os_image(vm);
		}
		vm_vcpus_run(vm);
	} else {
		ZVM_LOG_WARN("The VM has a invalid status, abort! \n");
        return -ENODEV;
	}

	ZVM_PRINTK("\n|*********************************************|\n");
	ZVM_PRINTK("|******\t Start vm successful!  ***************| \n");
	ZVM_PRINTK("|******\t\t VM INFO \t \t******| \n");
	ZVM_PRINTK("|******\t VM-NAME:     %s \t******| \n", vm->vm_name);
	ZVM_PRINTK("|******\t VM-ID: \t %d \t\t******| \n", vm->vmid);
	ZVM_PRINTK("|******\t VCPU NUM: \t %d \t\t******| \n", vm->vcpu_num);
	ZVM_PRINTK("|*********************************************|\n");

	return ret;
}


int zvm_pause_guest(size_t argc, char **argv)
{
	uint16_t vm_id;
	int ret = 0;
	struct vm *vm;
	k_spinlock_key_t key;

    key = k_spin_lock(&zvm_overall_info->spin_zmi);

	vm_id = z_parse_pause_vm_args(argc, argv, state);
	if (!(BIT(vm_id) & zvm_overall_info->alloced_vmid)) {
        ZVM_LOG_WARN("This vmid is not exist!\n Please input zvm info to show info! \n");
		k_spin_unlock(&zvm_overall_info->spin_zmi, key);
		return -EINVAL;
    }

	vm = zvm_overall_info->vms[vm_id];
	k_spin_unlock(&zvm_overall_info->spin_zmi, key);
	if (vm->vm_status != VM_STATE_RUNNING) {
		ZVM_LOG_WARN("This vm is not running!\n No need to pause it! \n");
		return -EPERM;
	}
	ret = vm_vcpus_pause(vm);

	return ret;
}


int zvm_delete_guest(size_t argc, char **argv)
{
	uint16_t vm_id;
	struct vm *vm;

	vm_id = z_parse_delete_vm_args(argc, argv, state);
	if (!(BIT(vm_id) & zvm_overall_info->alloced_vmid)) {
        ZVM_LOG_WARN("This vm is not exist!\n Please input zvm info to list vms!");
		return 0;
    }

	vm = zvm_overall_info->vms[vm_id];
	switch (vm->vm_status) {
	case VM_STATE_RUNNING:
		ZVM_PRINTK("This vm is running!\n Try to stop and delete it!\n");
		vm_vcpus_halt(vm);
		break;
	case VM_STATE_PAUSE:
		ZVM_PRINTK("This vm is paused!\n Just delete it!\n");
		vm_delete(vm);
		break;
	case VM_STATE_NEVER_RUN:
		ZVM_PRINTK("This vm is created but not run!\n Just delete it!\n");
		vm_delete(vm);
		break;
	default:
		ZVM_LOG_WARN("This vm status is invalid!\n");
		return -ENODEV;
	}

	return 0;
}


int zvm_info_guest(size_t argc, char **argv)
{
	uint16_t vm_id;
	int ret = 0;

	vm_id = z_parse_info_vm_args(argc, argv, state);
	if (!(BIT(vm_id) & zvm_overall_info->alloced_vmid) && vm_id != CONFIG_MAX_VM_NUM) {
        ZVM_LOG_WARN("This vm is not exist!\n Please input zvm info to list vms! \n");
		return -ENODEV;
    }
	if (zvm_overall_info->vm_total_num > 0) {
		ret = z_list_vms_info(vm_id);
	}else{
		ret = -ENODEV;
	}

	return ret;
}
