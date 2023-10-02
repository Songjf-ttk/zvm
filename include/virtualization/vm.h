/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VM_VM_H_
#define ZEPHYR_INCLUDE_ZVM_VM_VM_H_

#include <toolchain/gcc.h>
#include <stddef.h>
#include <stdint.h>
#include <kernel/thread.h>
#include <kernel_structs.h>

#include <virtualization/zvm.h>
#include <virtualization/os/os.h>
#include <virtualization/vm_irq.h>
#include <virtualization/arm/mm.h>
#include <virtualization/arm/cpu.h>
#include <virtualization/vm_mm.h>

#define DEFAULT_VM         (0)

#define VM_NAME_LEN         32
#define VCPU_NAME_LEN       32
#define RAMDISK_NAME_LEN    32

#define VCPU_THREAD_STACKSIZE 8192
#define VCPU_THREAD_PRIO    (1)

/**
 * @brief VM Status.
 */
#define VM_STATE_NEVER_RUN      (BIT(0))
#define VM_STATE_RUNNING        (BIT(1))
#define VM_STATE_PAUSE          (BIT(2))
#define VM_STATE_HALT           (BIT(3))

/**
 * @brief VM return values.
 */
#define VM_IRQ_TO_VM_SUCCESS        (1)
#define VM_IRQ_TO_VCPU_SUCCESS      (2)
#define VM_IRQ_NUM_OUT              (99)

#define _VCPU_STATE_READY        (BIT(0))
#define _VCPU_STATE_RUNNING      (BIT(1))
#define _VCPU_STATE_PAUSED       (BIT(2))
#define _VCPU_STATE_HALTED       (BIT(3))
#define _VCPU_STATE_UNKNOWN      (BIT(4))
#define _VCPU_STATE_RESET        (BIT(5))

#define VCPU_THREAD(thread)  ((struct k_thread *)thread->vcpu_struct ? true: false)

#define _current_vcpu _current->vcpu_struct

#define get_current_vcpu_id()       \
({                                  \
    struct vcpu *vcpu = (struct vcpu *)_current_vcpu; \
    vcpu->vcpu_id;                       \
})

#define get_current_vm()                                 \
({                                                        \
    struct vcpu *vcpu = (struct vcpu *)_current_vcpu; \
    vcpu->vm;                                               \
})

#define vcpu_need_switch(tid1, tid2) ((VCPU_THREAD(tid1)) || (VCPU_THREAD(tid2)))

struct vcpu_work;
struct z_vm_info;
struct os;
struct getopt_state;

struct zvm_host_data {
    struct zvm_vcpu_context host_ctext;
    struct k_spinlock hdata_lock;
};

/**
 * @brief Information describes vcpu.
 *
 * version: V1.0:
 *    Just support one vcpu bind to one thread.
 *
 *  @TODO We support SMP later.
 */
struct vcpu {
    struct vcpu_arch *arch;
    bool resume_signal;
    bool waitq_flag;
    uint16_t vcpu_id;

    /* corresponding physical cpu */
    uint16_t cpu;
    uint16_t vcpu_state;

    struct vm *vm;
    struct vcpu_work *work;

    /* vcpu timers record*/
    uint32_t hcpu_cycles;
    uint32_t runnig_cycles;
    uint32_t paused_cycles;

    /* vcpu's thread wait queue */
    _wait_q_t *t_wq;

    /* virq struct for this vcpu */
    struct virq_struct *virq_struct;
    struct _dnode vcpu_lists;
    struct vcpu *next_vcpu;
};
typedef struct vcpu vcpu_t;

/**
 * @brief  Describes the thread that vcpu binds to.
 */
struct __aligned(4) vcpu_work {
    /* statically allocate stack space */
    K_KERNEL_STACK_MEMBER(vt_stack, VCPU_THREAD_STACKSIZE);
    struct k_thread *vcpu_thread;

    /* point to vcpu struct */
    void *v_date;
};

/**
 * @brief The initial information used to create the virtual machine.
 */
struct vm_desc {
	uint16_t vmid;
	char name[VM_NAME_LEN];

    char vm_dtb_image_name[RAMDISK_NAME_LEN];
    char vm_kernel_image_name[RAMDISK_NAME_LEN];

	int32_t vcpu_num;
	uint64_t mem_base;
	uint64_t mem_size;

    /* vm's code entry */
	uint64_t entry;

    /* vm's states*/
	uint64_t flags;
	uint64_t image_load_address;
};

/**
 * @brief Record vm's vcpu num.
 * Recommend:Consider deleting
 */
struct vm_vcpu_num {
    uint16_t totle_vcpu_id;
    struct k_spinlock vcpu_id_lock;
};

struct vm_arch {
    uint64_t vm_pgd_base;
	uint64_t vttbr;
    uint64_t vtcr_el2;
};

/**
 * @brief Information describes guest vm that already exists.
 * Recommend:
 * 1. **vcpus change to **vcpus_list
 * 2. delete struct vm_vcpu_num vm_vcpu_id
 */
struct vm {
    bool is_rtos;
    uint16_t vmid;
    char vm_name[VM_NAME_LEN];

    uint32_t vm_status;
	uint32_t vcpu_num;
    uint32_t vtimer_offset;
    struct vcpu **vcpus;

    /* virtual memory space that allocate to this vm */
    struct vm_mem_domain *vmem_domain;
    struct os *os;

    /* bind the vm and the os type ops */
    struct zvm_ops *ops;
    struct vm_arch *arch;
    sys_dlist_t vdev_list;
    struct vm_vcpu_num vm_vcpu_id;

    /* vm irq control block */
    void *vm_irq_block_data;
    struct k_spinlock spinlock;
};

int vm_ops_init(struct vm *vm);

/**
 * @brief Init guest vm memory manager:
 * this function aim to init vm's memory manger,for below step:
 * 1. allocate virt space to vm(base/size), and distribute vpart_list to it.
 * 2. add this vpart to mapped_vpart_list.
 * 3. divide vpart area to block and init block list,
 * then allocate physical space to these block.
 * 4. build page table from vpart virt address to block physical address.
 *
 * @param vm: The vm that need to init mem
 * @return int 0 for success
 */
int vm_mem_init(struct vm *vm);

int vm_vcpus_create(uint16_t vcpu_num, struct vm *vm);
int vm_vcpus_init(struct vm *vm);
int vm_vcpus_run(struct vm *vm);
int vm_vcpus_pause(struct vm *vm);
int vm_vcpus_halt(struct vm *vm);
int vm_irq_block_create(struct vm *vm);
int vm_delete(struct vm *vm);

/**
 * @brief Parse vm ops args to get input info.
 *
 * @param state : opt struct.
 * @return int : error code.
 */
int z_parse_new_vm_args(size_t argc, char **argv, struct getopt_state *state,
                    struct z_vm_info *vm_info, struct vm *vm);

int z_parse_run_vm_args(size_t argc, char **argv, struct getopt_state *state);
int z_parse_pause_vm_args(size_t argc, char **argv, struct getopt_state *state);
int z_parse_delete_vm_args(size_t argc, char **argv, struct getopt_state *state);
int z_parse_info_vm_args(size_t argc, char **argv, struct getopt_state *state);

int z_list_vms_info(uint16_t vmid);

/**
 * @brief Init vm.
 *
 * @param vm: the vm ready to init.
 * @return int : error code.
 */
int vm_sysinfo_init(size_t argc, char **argv, struct vm **vm_ptr, struct getopt_state *state,
                struct z_vm_info **vm_info_ptr);

/**
 * @brief Process vm exit for pause or delete vm now.
 */
int vm_ipi_handler(struct vm *vm);

#endif /* ZEPHYR_INCLUDE_ZVM_VM_VM_H_ */
