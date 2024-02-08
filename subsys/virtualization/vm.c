/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain/gcc.h>

#include <kernel.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ksched.h>
#include <kernel/thread.h>
#include <kernel/thread_stack.h>
#include <kernel_internal.h>
#include <sys/mem_manage.h>
#include <sys/dlist.h>
#include <errno.h>
#include <logging/log.h>
#include <ksched.h>

#include <virtualization/arm/mm.h>
#include <virtualization/arm/cpu.h>
#include <virtualization/vdev/vgic_v3.h>
#include <virtualization/vm_mm.h>
#include <virtualization/zvm.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);


static int intra_vm_msg_handler(struct vm *vm)
{
    ARG_UNUSED(vm);
    return 0;
}

static int pause_vm_handler(struct vm *vm)
{
    ARG_UNUSED(vm);
    return 0;
}

static int stop_vm_handler(struct vm *vm)
{
    ARG_UNUSED(vm);
    return 0;
}

static void z_list_vm_info(uint16_t vmid)
{
    char *vm_ss;
    int mem_size = 0;
    struct vm *vm = zvm_overall_info->vms[vmid];

    if (!vm) {
        ZVM_LOG_WARN("Invalid vmid!\n");
        return;
    }

    /* Get the vm's status */
    switch (vm->vm_status) {
    case VM_STATE_RUNNING:
        vm_ss = "running";
        break;
    case VM_STATE_PAUSE:
        vm_ss = "pausing";
        break;
    case VM_STATE_NEVER_RUN:
        vm_ss = "Ready";
        break;
    case VM_STATE_HALT:
        vm_ss = "stopping";
        break;
    default:
        ZVM_LOG_WARN("This vm status is invalid!\n");
        return;
	}

    mem_size = vm->os->os_mem_size / (1024*1024);
    printk("|***%d  %s\t%d\t%d \t%s ***| \n", vm->vmid,
            vm->vm_name, vm->vcpu_num, mem_size, vm_ss);

}

static void z_list_all_vms_info(void)
{
    uint16_t i;

    printk("\n|******************** All VMS INFO *******************|\n");
    printk("|***vmid name \t    vcpus    vmem(M)\tstatus ***|\n");
    for(i = 0; i < CONFIG_MAX_VM_NUM; i++){
        if(BIT(i) & zvm_overall_info->alloced_vmid)
            z_list_vm_info(i);
    }
}

int vm_ipi_handler(struct vm *vm)
{
    int ret;
    uint32_t vm_status;

    vm_status = vm->vm_status;
    switch (vm_status) {
    case VM_STATE_RUNNING:
        ret = intra_vm_msg_handler(vm);
        break;
    case VM_STATE_PAUSE:
        ret = pause_vm_handler(vm);
        break;
    case VM_STATE_HALT:
        ret = stop_vm_handler(vm);
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

int vm_mem_init(struct vm *vm)
{
    int ret = 0;
    struct vm_mem_domain *vmem_dm = vm->vmem_domain;

    if (vmem_dm->is_init) {
        ZVM_LOG_WARN("VM's mem has been init before! \n");
        return -EMMAO;
    }

    ret = vm_mem_domain_partitions_add(vmem_dm);
    if (ret) {
        ZVM_LOG_WARN("Add partition to domain failed!, Code: %d \n", ret);
        return ret;
    }

    return 0;
}

int vm_create(struct z_vm_info *vm_info, struct vm *new_vm)
{
    int ret = 0;
    struct vm *vm = new_vm;

    /* init vmid here, this vmid is for vm level*/
    vm->vmid = allocate_vmid(vm_info);
    if (vm->vmid >= CONFIG_MAX_VM_NUM) {
        return -EOVERFLOW;
    }

    vm->os = (struct os *)k_malloc(sizeof(struct os));
	if (!vm->os) {
		ZVM_LOG_WARN("Allocate memory for os error! \n");
		return -ENOMEM;
	}

	ret = vm_os_create(vm, vm_info);
    if (ret) {
        ZVM_LOG_WARN("Unknow os type! \n");
        return ret;
    }

    ret = vm_mem_domain_create(vm);
    if (ret) {
        ZVM_LOG_WARN("vm_mem_domain_create failed! \n");
        return ret;
    }

    ret = vm_vcpus_create(vm_info->vcpu_num, vm);
    if (ret) {
        ZVM_LOG_WARN("vm_vcpus_create failed!");
        return ret;
    }

	vm->arch = (struct vm_arch *)k_malloc(sizeof(struct vm_arch));
	if (!vm->arch) {
		ZVM_LOG_WARN("Allocate mm memory for vm arch struct failed!");
		return -EMMAO;
	}

    vm->ops = (struct zvm_ops *)k_malloc(sizeof(struct zvm_ops));
    if (!vm->ops) {
        ZVM_LOG_WARN("Allocate mm memory for vm ops struct failed!");
        return -EMMAO;
    }

    vm->vm_vcpu_id.totle_vcpu_id = 0;
    ZVM_SPINLOCK_INIT(&vm->vm_vcpu_id.vcpu_id_lock);
    ZVM_SPINLOCK_INIT(&vm->spinlock);

    char vmid_str[4];
    uint16_t vmid_str_len = sprintf(vmid_str, "-%d", vm->vmid);
    if (vmid_str_len > 4) {
        ZVM_LOG_WARN("Sprintf put error, may cause str overflow!\n");
        vmid_str[3] = '\0';
    } else {
        vmid_str[vmid_str_len] = '\0';
    }

    if (strcpy(vm->vm_name, vm->os->name) == NULL || strcat(vm->vm_name, vmid_str) == NULL) {
        ZVM_LOG_WARN("VM name init error! \n");
        return -EIO;
    }

    /* set vm status here */
    vm->vm_status = VM_STATE_NEVER_RUN;

    /* store zvm overall info */
    zvm_overall_info->vms[vm->vmid] = vm;
    vm->arch->vm_pgd_base = (uint64_t)
            vm->vmem_domain->vm_mm_domain->arch.ptables.base_xlat_table;

    return 0;
}

int vm_ops_init(struct vm *vm)
{
    /* According to OS type to bind vm_ops. We need to add operation func@TODO*/
    return 0;
}

/**
 * @brief Get the vmid by id object.
 */
static uint16_t get_vmid_by_id(size_t argc, char **argv, struct getopt_state *state)
{
    uint16_t vm_id =  CONFIG_MAX_VM_NUM;
    int opt;
    char *optstring = "t:n:";

    /* Current exception level is EL2, parse input args.*/
	if (state == NULL) {
		/*
		 * If state is NULL, which means this is first time to use this struct.
		 * Then we should to initialize it by allocate memory for it. Otherwise,
		 * it means state has been used before and there is some specific value
		 * in it.
		 */
		state = (struct getopt_state*)k_malloc(sizeof(struct getopt_state));
		if (!state) {
			ZVM_LOG_WARN("Allocation memory for getopt_state Error! \n");
			return -ENOMEM;
		}
	}
	getopt_init(state);

    while ((opt = getopt(state, argc, argv, optstring)) != -1) {
		switch (opt) {
		case 'n':
            vm_id = (uint16_t)(state->optarg[0] - '0');
        /*  do not support sscanf
            err = sscanf(state->optarg, "%d", &vm_id);
            if(!err){
                ZVM_LOG_WARN("Get vm_id error.");
			    return -EINVAL;
            }
        */
			break;
		default:
			ZVM_LOG_WARN("Input number invalid, Please input a valid vmid after \"-n\" command! \n");
			return -EINVAL;
		}
	}
    return vm_id;
}

int vm_vcpus_create(uint16_t vcpu_num, struct vm *vm)
{
    /* init vcpu num */
    if (vcpu_num > CONFIG_MAX_VCPU_PER_VM) {
        vcpu_num = CONFIG_MAX_VCPU_PER_VM;
    }
    vm->vcpu_num = vcpu_num;

    /* allocate vcpu list here */
    vm->vcpus = (struct vcpu **)k_malloc(vcpu_num * sizeof(struct vcpu *));
    if (!(vm->vcpus)) {
        ZVM_LOG_WARN("Vcpus struct init error !\n");
        return -EMMAO;
    }
    return 0;
}

int vm_vcpus_init(struct vm *vm)
{
    char vcpu_name[VCPU_NAME_LEN];
    int i;
    struct vcpu *vcpu;

    if (vm->vcpu_num > CONFIG_MAX_VCPU_PER_VM) {
        ZVM_LOG_WARN("Vcpu counts is too big!");
        return -EPANO;
    }

    for(i = 0; i < vm->vcpu_num; i++){
        memset(vcpu_name, 0, VCPU_NAME_LEN);
        snprintk(vcpu_name, VCPU_NAME_LEN-1, "%s-vcpu%d", vm->vm_name, i);

        vcpu = vm_vcpu_init(vm, i, vcpu_name);

        sys_dlist_init(&vcpu->vcpu_lists);
        vm->vcpus[i] = vcpu;
        vcpu->next_vcpu = NULL;

        if (i) {
            vm->vcpus[i-1]->next_vcpu = vcpu;
        }
    }

    return 0;
}

int vm_vcpus_run(struct vm *vm)
{
    uint16_t i=0;
    struct vcpu *vcpu;
    struct k_thread *thread;
    k_spinlock_key_t key;
    ARG_UNUSED(thread);

    key = k_spin_lock(&vm->spinlock);
    for(i = 0; i < vm->vcpu_num; i++){
        /* find the vcpu struct */
        vcpu = vm->vcpus[i];
        if (vcpu == NULL) {
            ZVM_LOG_WARN("vm error here, can't find vcpu: vcpu-%d", i);
            k_spin_unlock(&vm->spinlock, key);
            return -ENODEV;
        }
        vm_vcpu_ready(vcpu);
    }
    vm->vm_status = VM_STATE_RUNNING;
    k_spin_unlock(&vm->spinlock, key);

    return 0;
}

int vm_vcpus_pause(struct vm *vm)
{
    uint16_t i=0;
    struct vcpu *vcpu;
    struct k_thread *thread, *cur_thread;
    k_spinlock_key_t key;
    ARG_UNUSED(thread);
    ARG_UNUSED(cur_thread);

    key = k_spin_lock(&vm->spinlock);
    for(i = 0; i < vm->vcpu_num; i++){
        vcpu = vm->vcpus[i];
        if (vcpu == NULL) {
            ZVM_LOG_WARN("Pause vm error here, can't find vcpu: vcpu-%d \n", i);
            k_spin_unlock(&vm->spinlock, key);
            return -ENODEV;
        }
        vm_vcpu_pause(vcpu);
    }

    vm->vm_status = VM_STATE_PAUSE;
    k_spin_unlock(&vm->spinlock, key);
    return 0;
}

int vm_vcpus_halt(struct vm *vm)
{
    uint16_t i=0;
    struct vcpu *vcpu;
    struct k_thread *thread;
    k_spinlock_key_t key;
    ARG_UNUSED(thread);

    key = k_spin_lock(&vm->spinlock);
    for(i = 0; i < vm->vcpu_num; i++){
        /* find the vcpu struct */
        vcpu = vm->vcpus[i];
        if (vcpu == NULL) {
            ZVM_LOG_WARN("vm error here, can't find vcpu: vcpu-%d", i);
            k_spin_unlock(&vm->spinlock, key);
            return -ENODEV;
        }
        vm_vcpu_halt(vcpu);
    }
    vm->vm_status = VM_STATE_HALT;
    k_spin_unlock(&vm->spinlock, key);

    return 0;
}

int vm_delete(struct vm *vm)
{
    int ret=0;

    struct _dnode *dev_list = &vm->vdev_list;
    struct  _dnode *d_node, *ds_node;
    struct virt_dev *vdev;
    k_spinlock_key_t key;
    struct vm_mem_domain *vmem_dm = vm->vmem_domain;
    struct vcpu *vcpu;
    struct vcpu_work *vwork;

    key = k_spin_lock(&vm->spinlock);

    /* delete vdev struct */
    SYS_DLIST_FOR_EACH_NODE_SAFE(dev_list, d_node, ds_node){
        vdev = CONTAINER_OF(d_node, struct virt_dev, vdev_node);
        if (vdev->dev_pt_flag) {
            /* remove vdev from vm. */
            /////ret = delete_vm_monopoly_vdev(vm, vdev);
            /* @TODO: May need to record the pass through's count*/
            vdev->dev_pt_flag = false;
        }
    }

    /* remove all the partition in the vmem_domain */
    ret = vm_mem_apart_remove(vmem_dm);

    /* delete vcpu struct */
    for(int i = 0; i < vm->vcpu_num; i++){
        vcpu = vm->vcpus[i];
        if(!vcpu) continue;
        vwork = vcpu->work;
        if(vwork){
            k_free(vwork->vcpu_thread);
        }

        k_free(vcpu->arch);
        k_free(vcpu->work);
        k_free(vcpu);
    }

    k_free(vm->ops);
    k_free(vm->arch);
    k_free(vm->vcpus);
    k_free(vm->vmem_domain);
    if(vm->os->name) k_free(vm->os->name);
    k_free(vm->os);
    zvm_overall_info->vms[vm->vmid] = NULL;
    k_free(vm);
    k_spin_unlock(&vm->spinlock, key);

    zvm_overall_info->vm_total_num--;
    zvm_overall_info->alloced_vmid &= ~BIT(vm->vmid);
    return 0;
}

int z_parse_new_vm_args(size_t argc, char **argv, struct getopt_state *state,
                    struct z_vm_info *vm_info, struct vm *vm)
{
    int ret = 0;
    int opt;
    char *optstring = "t:n:";

    /* Current exception level is EL2, parse input args.*/
	if (state == NULL) {
		state = (struct getopt_state*)k_malloc(sizeof(struct getopt_state));
		if (!state) {
			ZVM_LOG_WARN("Allocation memory for getopt_state Error! \n");
			return -ENOMEM;
		}
	}
	getopt_init(state);

    while ((opt = getopt(state, argc, argv, optstring)) != -1) {
		switch (opt) {
		case 't':
			ret = get_os_info_by_type(state, vm_info);
			continue;
		case 'n':
            /* @TODO: support allocate vmid chosen by user later */
		default:
            ZVM_LOG_WARN("Input error! \n");
			ZVM_LOG_WARN("Please input \" zvm new -t + os_name \" command to new a vm! \n");
			return -EINVAL;
		}
	}

    return ret;
}

/**
 * @brief Parse run vm args here. get the vmid that which vm need to process. 
 */
int z_parse_run_vm_args(size_t argc, char **argv, struct getopt_state *state)
{
    return get_vmid_by_id(argc, argv, state);
}

int z_parse_pause_vm_args(size_t argc, char **argv, struct getopt_state *state)
{
    return get_vmid_by_id(argc, argv, state);
}

int z_parse_delete_vm_args(size_t argc, char **argv, struct getopt_state *state)
{
    return get_vmid_by_id(argc, argv, state);
}

int z_parse_info_vm_args(size_t argc, char **argv, struct getopt_state *state)
{
    return get_vmid_by_id(argc, argv, state);
}

int z_list_vms_info(uint16_t vmid)
{
    /* if vmid equal to CONFIG_MAX_VM_NUM, list all vm */
    if (vmid == CONFIG_MAX_VM_NUM) {
        z_list_all_vms_info();
    } else {
        printk("\n|*********************** VM INFO *********************|\n");
        printk("|***vmid name \t    vcpus    vmem(M)\tstatus ***|\n");
        z_list_vm_info(vmid);
    }
    printk("|*****************************************************|\n");
    return 0;
}

int vm_sysinfo_init(size_t argc, char **argv, struct vm **vm_ptr, struct getopt_state *state,
                struct z_vm_info **vm_info_ptr)
{
    int ret = 0;
    struct vm *vm = NULL;
    struct z_vm_info *vm_info = NULL;

    /* allocate vm struct */
    vm = (struct vm*)k_malloc(sizeof(struct vm));
	if (!vm) {
		ZVM_LOG_WARN("Allocation memory for VM Error!\n");
		return -ENOMEM;
	}

    /* allocate vm_info struct */
    vm_info = (struct z_vm_info *)k_malloc(sizeof(struct z_vm_info));
	if (!vm_info) {
        k_free(vm);
		ZVM_LOG_WARN("Allocation memory for VM info Error!\n");
		return -ENOMEM;
	}

    ret = z_parse_new_vm_args(argc, argv, state, vm_info, vm);
	if (ret) {
		k_free(vm);
		k_free(vm_info);
		return ret;
	}

    *vm_ptr = vm;
    *vm_info_ptr = vm_info;

    return ret;
}
