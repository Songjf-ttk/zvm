/*
 * Copyright 2021-2022 HNU
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VM_IRQ_H_
#define ZEPHYR_INCLUDE_ZVM_VM_IRQ_H_

#include <stdint.h>
#include <sys/dlist.h>
#include <arch/arm64/lib_helpers.h>

#include <_zvm/zvm.h>
#include <_zvm/arm/trap_handler.h>

/* Software irq flag */
#define VIRQ_PENDING_FLAG		BIT(0)
#define VIRT_IRQ_HW             BIT(1)
#define VIRT_IRQ_ENABLED		BIT(2)
#define VIRT_IRQ_SUSPEND		BIT(3)
#define VIRT_IRQ_REMOVED        BIT(4)
#define VIRT_IRQ_IS_WAKEUP      BIT(5)
#define VIRT_IRQ_NOUSED         BIT(6)

/* VM's injuct irq num, bind to the register */
#define VM_VIRT_SGI_IRQ     (0x01)
#define VM_VIRT_CONSOLE_IRQ (0x02)
#define VIRT_IRQ_INVALID_ID	(0xFF)

/* For clear warning for unknow reason */
struct vm;
struct vcpu;

/**
 * @brief Description for each virt irq.
 */
struct virt_irq_desc {
    uint8_t id;
    uint8_t vcpu_id;
    uint8_t vm_id;
    uint8_t virq_states;
    uint8_t prio;

    /* irq level type */
    uint8_t type;
    uint8_t src_cpu;

    /* virt_irq flags */
    uint32_t virq_flags;

    uint32_t pirq_num;
    uint32_t virq_num;

    sys_dnode_t desc_node;
};

/**
 * @brief Vcpu irq infomation.
 */
struct virq_struct{
    /* active virq */
    uint32_t virq_act_counts;

    struct virt_irq_desc vcpu_virt_irq_desc[VM_LOCAL_VIRQ_NR];

    sys_dlist_t active_irqs;
    sys_dlist_t pending_irqs;

    struct k_spinlock spinlock;
};

/**
 * @brief Get virq control block.
 */
int vm_virq_block_desc_init(struct vm *vm, void *args);

/**
 * @brief Get irq block for vm.
 */
int vm_irq_ctlblock_init(struct vm *vm);

#endif /* ZEPHYR_INCLUDE_ZVM_VM_IRQ_H_ */
