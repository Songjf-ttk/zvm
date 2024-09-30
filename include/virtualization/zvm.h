/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_H_
#define ZEPHYR_INCLUDE_ZVM_H_

#include <stdint.h>
#include <devicetree.h>
#include <errno.h>
#include <spinlock.h>
#include <logging/log.h>
#include <sys/dlist.h>
#include <sys/printk.h>
#include <toolchain/common.h>
#include <sys/util.h>
#include <virtualization/os/os.h>

#ifndef CONFIG_ZVM
#define CONFIG_ZVM
#endif  /* CONFIG_ZVM */
#ifndef CONFIG_MAX_VM_NUM
#define CONFIG_MAX_VM_NUM  (0)
#endif /* CONFIG_MAX_VM_NUM */

#define ZVM_MODULE_NAME zvm_host
#define SINGLE_CORE     1U
#define DT_MB           (1024 * 1024)

/* @TODO: delete some ugly inlcude file later */
#if defined(CONFIG_ZVM)
#include <virtualization/vm.h>
#include <virtualization/vm_mm.h>
#include <virtualization/vm_manager.h>
#include <virtualization/vm_cpu.h>
#else
/* No thing to do here */
#endif /* CONFIG_ZVM */

#ifdef CONFIG_ZVM

/* @TODO: support for more error types */
#define EMMAO   ENXIO       /* Not enough memory */
#define ENOVDEV ENODEV      /* Virtual source load error */
#define EVMODE  (150)       /* Not on hypervisor mode */
#define EPANO   (155)       /* Erorr on vm input parameter */
#define EVIRQ   (156)       /* VM's virq error */
#define EVMOS   (157)       /* VM's os type error */
#endif

#define ZVM_EXIT_UNKNOWN            (11)
#define ZVM_DEV_NAME_LENGTH         (32)

/**
 * @brief We should define overall priority for zvm system.
 * A total of 15 priorities are defined by setting
 * CONFIG_NUM_PREEMPT_PRIORITIES = 15 and can be divided into three categories:
 * 1    ->   5: high real-time requirement and very critical to the system.
 * 6    ->  10: no real-time requirement and very critical to the system.
 * 10   ->  15: normal.
 */
#define RT_VM_WORK_PRIORITY     (5)
#define NORT_VM_WORK_PRIORITY   (10)

#ifdef CONFIG_LOG
#define ZVM_LOG_ERR(...)    LOG_ERR(__VA_ARGS__)
#define ZVM_LOG_WARN(...)   LOG_WRN(__VA_ARGS__)

/* print system log info */
#ifdef CONFIG_ZVM_DEBUG_LOG_INFO
#define ZVM_LOG_INFO(...)   LOG_PRINTK(__VA_ARGS__)
#else
#define ZVM_LOG_INFO(...)
#endif
#define ZVM_PRINTK(...)     LOG_PRINTK(__VA_ARGS__)

#else

#define ZVM_LOG_ERR(format, ...) \
do {\
    DEBUG("\033[36m[ERR:]File:%s Line:%d. " format "\n\033[0m", __FILE__, \
            __LINE__, ##__VA_ARGS__);\
} while(0);
#define ZVM_LOG_WARN(format, ...) \
do {\
    DEBUG("\033[36m[WRN:]File:%s Line:%d. " format "\n\033[0m", __FILE__, \
            __LINE__, ##__VA_ARGS__);\
} while(0);
#define ZVM_LOG_INFO(...)   printk(__VA_ARGS__)

#endif


/**
 * @brief Spinlock initialization for smp
 */
#if defined(CONFIG_SMP) && defined(CONFIG_SPIN_VALIDATE)
#define ZVM_SPINLOCK_INIT(dev)  \
({                                      \
    struct k_spinlock *lock = (struct k_spinlock *)dev; \
	lock->locked = 0x0;                 \
    lock->thread_cpu = 0x0;      \
})
#elif defined(CONFIG_SMP) && !defined(CONFIG_SPIN_VALIDATE)
#define ZVM_SPINLOCK_INIT(dev)  \
({                                      \
    struct k_spinlock *lock = (struct k_spinlock *)dev; \
	lock->locked = 0x0;                 \
})
#else
#define ZVM_SPINLOCK_INIT(dev)
#endif

extern struct z_kernel _kernel;
extern struct zvm_manage_info *zvm_overall_info;

/* ZVM's functions */
typedef     void (*zvm_new_vm_t)(size_t argc, char **argv);
typedef     void (*zvm_run_vm_t)(uint32_t vmid);
typedef     void (*zvm_update_vm_t)(size_t argc, char **argv);
typedef     void (*zvm_info_vm_t)(size_t argc, char **argv);
typedef     void (*zvm_pause_vm_t)(uint32_t vmid);
typedef     void (*zvm_halt_vm_t)(size_t argc, char **argv);
typedef     void (*zvm_delete_vm_t)(uint32_t vmid);


/**
 * @brief zvm_hwsys_info stores basic information about the ZVM.
 *
 * What hardware resource we concern about now includes CPU and memory(named
 * sram in dts file), such as CPU's compatible property and memory size. Other
 * devices we do not care currently. Then we need a structure to store basic
 * information of hardware.
 */
struct zvm_hwsys_info {
    char *cpu_type;
    uint16_t phy_cpu_num;
    uint64_t phy_mem;
    uint64_t phy_mem_used;
};

/**
 * @brief General operations for virtual machines.
 */
struct zvm_ops {
    zvm_new_vm_t            new_vm;
    zvm_run_vm_t            run_vm;
    zvm_update_vm_t         update_vm;
    zvm_info_vm_t           info_vm;
    zvm_pause_vm_t          pause_vm;
    zvm_halt_vm_t           halt_vm;
    zvm_delete_vm_t         delete_vm;
};

/**
 * .entry_point: vm code entry point;
 * .vm_virt_base: virt base addredd of vm, In general,
 * virt base address should not be a static address, It should
 * allocated by host os.
 * .vm_image_base: os's image base in the disk;
 * .vm_image_size: os's image size in the disk;
*/
struct z_vm_info {
    uint16_t    vm_os_type;
    uint16_t    vcpu_num;
    uint64_t    vm_image_base;
    uint64_t    vm_image_size;
    uint64_t    vm_virt_base;
    uint64_t    vm_sys_size;
    uint64_t    entry_point;
};
typedef struct z_vm_info z_vm_info_t;

/*
 * @brief ZVM manage structure.
 *
 * As a hypervisor, zvm should know how much resouce it can use and how many vm
 * it carries.
 * At first aspect, File subsys/_zvm_zvm_host/zvm_host.c can get hardware info
 * from devicetree file. We construct corresponding data structure type
 * "struct zvm_hwsys_info" to store it. "struct zvm_hwsys_info" includes:
 *  -> the number of total vm
 *  -> the number of physical CPU
 *  -> system's CPU typename
 *  -> the number of physical memory
 *  -> how much physical memory has been used
 * and so on.
 * At second aspect, we should konw what kind of resource vm possess is proper.
 * Then we construct a proper data structure, just like "vm_info_t", to describe
 * it. It includes information as below:
 *  -> ...
 */
struct zvm_manage_info {

    /* The hardware infomation of this device */
    struct zvm_hwsys_info *hw_info;

    struct vm *vms[CONFIG_MAX_VM_NUM];

    /* @TODO: try to add a flag to describe the running vm and pending vm list */

    /** Each bit of this value represents a virtual machine id.
     * When the value of a bit is 1,
     * the ID of that virtual machine has been allocated, and vice versa.
     */
    uint32_t alloced_vmid;

    /* total num of vm in system */
    uint32_t vm_total_num;
    struct k_spinlock spin_zmi;
};

void zvm_ipi_handler(void);
void zvm_info_print(struct zvm_hwsys_info *sys_info);
struct zvm_dev_lists* get_zvm_dev_lists(void);

int vm_create(struct z_vm_info *zvi, struct vm *vm);
int load_os_image(struct vm *vm);

static uint32_t used_cpus = 0;
static struct k_spinlock cpu_mask_lock;

static ALWAYS_INLINE int rt_get_idle_cpu(void) {
    for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
#ifdef CONFIG_SMP
        /* In SMP, _current is a field read from _current_cpu, which
        * can race with preemption before it is read.  We must lock
        * local interrupts when reading it.
        */
	    unsigned int k = arch_irq_lock();
#endif
        k_tid_t tid = _kernel.cpus[i].current;
#ifdef CONFIG_SMP
	    arch_irq_unlock(k);
#endif
        int prio = k_thread_priority_get(tid);
        if (prio == K_IDLE_PRIO || (prio < K_IDLE_PRIO && prio > VCPU_RT_PRIO)) {
            return i;
        }
    }
    return -ESRCH;
}

static ALWAYS_INLINE int nrt_get_idle_cpu(void) {
    for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
#ifdef CONFIG_SMP
        /* In SMP, _current is a field read from _current_cpu, which
        * can race with preemption before it is read.  We must lock
        * local interrupts when reading it.
        */
        unsigned int k = arch_irq_lock();
#endif
        k_tid_t tid = _kernel.cpus[i].current;
#ifdef CONFIG_SMP
	    arch_irq_unlock(k);
#endif
        int prio = k_thread_priority_get(tid);
        if (prio == K_IDLE_PRIO) {
            return i;
        }
    }
    return -ESRCH;
}

static ALWAYS_INLINE int get_static_idle_cpu(void) {
    k_spinlock_key_t key;

    for (int i = 1; i < CONFIG_MP_NUM_CPUS; i++) {
#ifdef CONFIG_SMP
        /* In SMP, _current is a field read from _current_cpu, which
        * can race with preemption before it is read.  We must lock
        * local interrupts when reading it.
        */
        unsigned int k = arch_irq_lock();
#endif
        k_tid_t tid = _kernel.cpus[i].current;
#ifdef CONFIG_SMP
        arch_irq_unlock(k);
#endif
        int prio = k_thread_priority_get(tid);
        if (prio == K_IDLE_PRIO && !(used_cpus & (1 << i))) {
            key = k_spin_lock(&cpu_mask_lock);
            used_cpus |= (1 << i);
            k_spin_unlock(&cpu_mask_lock, key);
            return i;
        }
    }
    return -ESRCH;
}

static ALWAYS_INLINE bool is_vmid_full(void)
{
    return zvm_overall_info->alloced_vmid == BIT_MASK(CONFIG_MAX_VM_NUM);
}

static ALWAYS_INLINE uint32_t find_next_vmid(struct z_vm_info *vm_info, uint32_t *vmid) {
    uint32_t id, maxid;

    if (vm_info->vm_os_type == OS_TYPE_ZEPHYR) {
        *vmid = 0;
        id = BIT(0);
        maxid = BIT(ZVM_ZEPHYR_VM_NUM);
    } else {
        *vmid = ZVM_ZEPHYR_VM_NUM;
        id = BIT(ZVM_ZEPHYR_VM_NUM);
        maxid = BIT(CONFIG_MAX_VM_NUM);
    }

    for (; id < maxid; id <<= 1,(*vmid)++) {
        if (!(id & zvm_overall_info->alloced_vmid)) {
            zvm_overall_info->alloced_vmid |= id;
            return 0;
        }
    }
    return -EOVERFLOW;
}

/**
 * @brief Allocate a unique vmid for this VM.
 * TODO: Need atomic op to vmid.
 */
static ALWAYS_INLINE uint32_t allocate_vmid(struct z_vm_info *vm_info) {
    int err;
    uint32_t res;
    k_spinlock_key_t key;

    if (unlikely(is_vmid_full())) {
        return CONFIG_MAX_VM_NUM;      /* Value overflow. */
    }

    key = k_spin_lock(&zvm_overall_info->spin_zmi);
    err = find_next_vmid(vm_info, &res);
    if (err) {
        k_spin_unlock(&zvm_overall_info->spin_zmi, key);
        return CONFIG_MAX_VM_NUM;
    }

    zvm_overall_info->vm_total_num++;

    k_spin_unlock(&zvm_overall_info->spin_zmi, key);

    return res;
}

static ALWAYS_INLINE struct vm *get_vm_by_id(uint32_t vmid) {
    if (unlikely(vmid >= CONFIG_MAX_VM_NUM)){
        return NULL;
    }
    return zvm_overall_info->vms[vmid];
}

#endif /* ZEPHYR_INCLUDE_ZVM_H_ */
