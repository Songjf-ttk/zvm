/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VM_MANAGER_H_
#define ZEPHYR_INCLUDE_ZVM_VM_MANAGER_H_

#include <stddef.h>
#include <errno.h>
#include <arch/arm64/cpu.h>
#include <shell/shell.h>
#include <sys/printk.h>

#include <virtualization/zvm.h>
#include <virtualization/arm/mm.h>
#include <virtualization/vm_mm.h>

#ifdef CONFIG_ZVM_TEST
#include <getopt.h>
#endif /* CONFIG_ZVM_TEST */

/* For clear warning for unknow reason */
struct vm_mem_domain;
struct vcpu;
struct vm;

/**
 * @brief init vm struct.
 * When creating a vm, we must load the vm image and parse it
 * if it is a complicated system.
 */
typedef void (*vm_init_t)(struct vm *vm);

typedef void (*vcpu_init_t)(struct vcpu *vcpu);
typedef void (*vcpu_run_t)(struct vcpu *vcpu);
typedef void (*vcpu_halt_t)(struct vcpu *vcpu);

/**
 * @brief VM mapping vir_ad to phy_ad.
 */
typedef int (*vm_mmap_t)(struct vm_mem_domain *vmem_domain);

typedef void (*vmm_init_t)(struct vm *vm);
typedef int (*vint_init_t)(struct vcpu *vcpu);
typedef int (*vtimer_init_t)(struct vcpu *vcpu);

/**
 * @brief VM's vcpu ops function
 */
struct vm_ops {
    vm_init_t       vm_init;

    vcpu_init_t     vcpu_init;
    vcpu_run_t      vcpu_run;
    vcpu_halt_t     vcpu_halt;

    vmm_init_t      vmm_init;
    vm_mmap_t       vm_mmap;

    /* @TODO maybe add load/restor func later */
    vint_init_t     vint_init;
    vtimer_init_t   vtimer_init;
};

/**
 * @file APIs which realted to vm.
 *
 * All vm command include create, set, run, update, list, halt and delete vm.
 * These APIs will be called by subsys/zvm/shell.c.
 */
int zvm_new_guest(size_t argc, char **argv);
int zvm_run_guest(size_t argc, char **argv);
int zvm_pause_guest(size_t argc, char **argv);
int zvm_delete_guest(size_t argc, char **argv);
int zvm_info_guest(size_t argc, char **argv);

/**
 * @brief set vm status from service vm.
 * This function will be called by service vm.
 * For example: vm is not running, we can set it to running.
 */
int zvm_service_vmops(uint32_t hypercall_code);

/**
 * @brief shutdown guest
 */
void zvm_shutdown_guest(struct vm *vm);

/**
 * @brief reset guset
 */
void zvm_reboot_guest(struct vm *vm);


#endif /* ZEPHYR_INCLUDE_ZVM_VM_MANAGER_H_ */
