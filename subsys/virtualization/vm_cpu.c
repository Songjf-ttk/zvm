/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <ksched.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>
#include <virtualization/zvm.h>
#include <virtualization/arm/cpu.h>
#include <virtualization/arm/switch.h>
#include <virtualization/arm/vtimer.h>
#include <virtualization/vm_dev.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);


/**
 * @brief Construct a new vcpu virt desc object
 */
static void init_vcpu_virt_irq_desc(struct virq_struct *virq_struct)
{
    int i;
    struct virt_irq_desc *desc;
    for(i = 0; i < VM_LOCAL_VIRQ_NR; i++){
        desc = &virq_struct->vcpu_virt_irq_desc[i];
        desc->id = 0;
        desc->pirq_num = i;
        desc->virq_num = i;
        desc->prio = 1;
        desc->vcpu_id = DEFAULT_VCPU;
        desc->virq_flags = 0;
        desc->virq_states = 0;
        desc->vm_id = DEFAULT_VM;

        sys_dnode_init(&(desc->desc_node));
    }
}

/**
 * @brief store vcpu context before switch to vcpu_thread.
 */
static void save_vcpu_context(struct k_thread *thread)
{
    arch_vcpu_context_save(thread->vcpu_struct);
}

/**
 * @brief store vcpu context before switch to vcpu_thread.
 */
static void load_vcpu_context(struct k_thread *thread)
{
    struct vcpu *vcpu = thread->vcpu_struct;

    arch_vcpu_context_load(thread->vcpu_struct);

    vcpu->resume_signal = false;
}

/**
 * @brief Construct a new vcpu timer event pause object
 */
static void vcpu_timer_event_pause(struct vcpu *vcpu)
{
    struct virt_timer_context *timer_ctxt = vcpu->arch->vtimer_context;

    z_abort_timeout(&timer_ctxt->vtimer_timeout);
    z_abort_timeout(&timer_ctxt->ptimer_timeout);
}

static void vcpu_context_switch(struct k_thread *new_thread,
            struct k_thread *old_thread)
{

    if (VCPU_THREAD(old_thread)) {
        struct vcpu *old_vcpu = old_thread->vcpu_struct;

        save_vcpu_context(old_thread);
        switch (old_vcpu->vcpu_state) {
        case _VCPU_STATE_RUNNING:
            old_vcpu->vcpu_state = _VCPU_STATE_READY;
            break;
        case _VCPU_STATE_RESET:
            ZVM_LOG_WARN("Do not support vm reset! \n");
            break;
        case _VCPU_STATE_PAUSED:
            vcpu_timer_event_pause(old_vcpu);
            vm_vdev_pause(old_vcpu);
            break;
        case _VCPU_STATE_HALTED:
            vcpu_timer_event_pause(old_vcpu);
            vm_vdev_pause(old_vcpu);
            break;
        default:
            break;
        }
    }

    if (VCPU_THREAD(new_thread)) {
        struct vcpu *new_vcpu = new_thread->vcpu_struct;

        if (new_vcpu->vcpu_state != _VCPU_STATE_READY) {
            ZVM_LOG_ERR("vCPU is not ready, something may be wrong.\n");
        }

        load_vcpu_context(new_thread);
        new_vcpu->vcpu_state = _VCPU_STATE_RUNNING;
    }

}

static void vcpu_state_to_ready(struct vcpu *vcpu)
{
    uint16_t cur_state = vcpu->vcpu_state;
    struct k_thread *thread = vcpu->work->vcpu_thread;

    switch (cur_state)
    {
    case _VCPU_STATE_READY:
        break;
    case _VCPU_STATE_RUNNING:
        vcpu->resume_signal = true;
        break;
    case _VCPU_STATE_RESET:
    case _VCPU_STATE_PAUSED:
        k_wakeup(thread);
        break;
    default:
        ZVM_LOG_WARN("Invalid cpu state here. \n");
        break;
    }

}

static void vcpu_state_to_running(struct vcpu *vcpu)
{
    ARG_UNUSED(vcpu);
    ZVM_LOG_WARN("No thing to do, running state may be auto switched. \n");
}

static void vcpu_state_to_reset(struct vcpu *vcpu)
{
    uint16_t cur_state = vcpu->vcpu_state;
    struct k_thread *thread = vcpu->work->vcpu_thread;

    switch (cur_state)
    {
    case _VCPU_STATE_READY:
        yield_thread(thread);
        break;
    case _VCPU_STATE_RESET:
        break;
    case _VCPU_STATE_RUNNING:
    case _VCPU_STATE_PAUSED:
        arch_vcpu_init(vcpu);
        break;
    default:
        ZVM_LOG_WARN("Invalid cpu state here. \n");
        break;
    }
    vcpu->resume_signal = false;

}

static void vcpu_state_to_paused(struct vcpu *vcpu)
{
    bool resumed = false;
    uint16_t cur_state = vcpu->vcpu_state;
    struct k_thread *thread = vcpu->work->vcpu_thread;

    switch (cur_state) {
    case _VCPU_STATE_READY:
    case _VCPU_STATE_RUNNING:
        resumed = vcpu->resume_signal;
        vcpu->resume_signal = false;
        if (resumed && vcpu->waitq_flag) {
            vcpu_timer_event_pause(vcpu);
        }
        thread->base.thread_state |= _THREAD_SUSPENDED;
        dequeue_ready_thread(thread);
        break;
    case _VCPU_STATE_RESET:
    case _VCPU_STATE_PAUSED:
    default:
        ZVM_LOG_WARN("Invalid cpu state. \n");
        break;
    }

}

static void vcpu_state_to_halted(struct vcpu *vcpu)
{
    uint16_t cur_state = vcpu->vcpu_state;
    struct k_thread *thread = vcpu->work->vcpu_thread;

    switch (cur_state) {
    case _VCPU_STATE_READY:
    case _VCPU_STATE_RUNNING:
    case _VCPU_STATE_PAUSED:
        thread->base.thread_state |= _THREAD_VCPU_NO_SWITCH;
        break;
    case _VCPU_STATE_RESET:
    case _VCPU_STATE_UNKNOWN:
        vm_delete(vcpu->vm);
        break;
    default:
        ZVM_LOG_WARN("Invalid cpu state here. \n");
        break;
    }
    vcpu_ipi_scheduler(VCPU_IPI_MASK_ALL,0);

}

static void vcpu_state_to_unknown(struct vcpu *vcpu)
{
    ARG_UNUSED(vcpu);
}

/**
 * @brief Vcpu scheduler for switch vcpu to different states.
 */
int vcpu_state_switch(struct k_thread *thread, uint16_t new_state)
{
    int ret = 0;
    struct vcpu *vcpu = thread->vcpu_struct;
    uint16_t cur_state = vcpu->vcpu_state;

    if (cur_state == new_state) {
        return ret;
    }
    switch (new_state) {
    case _VCPU_STATE_READY:
        vcpu_state_to_ready(vcpu);
        break;
    case _VCPU_STATE_RUNNING:
        vcpu_state_to_running(vcpu);
        break;
    case _VCPU_STATE_RESET:
        vcpu_state_to_reset(vcpu);
        break;
    case _VCPU_STATE_PAUSED:
        vcpu_state_to_paused(vcpu);
        break;
    case _VCPU_STATE_HALTED:
        vcpu_state_to_halted(vcpu);
        break;
    case _VCPU_STATE_UNKNOWN:
        vcpu_state_to_unknown(vcpu);
        break;
    default:
        ZVM_LOG_ERR("Invalid state here. \n");
        ret = EINVAL;
        break;
    }
    vcpu->vcpu_state = new_state;

    return ret;

}

void do_vcpu_swap(struct k_thread *new_thread, struct k_thread *old_thread)
{
    uint32_t cur_state;
    struct vcpu *vcpu;
    ARG_UNUSED(cur_state);
    ARG_UNUSED(vcpu);

#ifdef CONFIG_SMP
    vcpu_context_switch(new_thread, old_thread);
#else
    if (old_thread && VCPU_THREAD(old_thread)) {
        save_vcpu_context(old_thread);
    }
    else if (new_thread && VCPU_THREAD(new_thread)) {
        load_vcpu_context(new_thread);
    }
#endif /* CONFIG_SMP */
}

void do_asm_vcpu_swap(struct k_thread *new_thread, struct k_thread *old_thread)
{
    if (!vcpu_need_switch(new_thread, old_thread)) {
        return ;
    }
    do_vcpu_swap(new_thread, old_thread);
}

int vcpu_ipi_scheduler(uint32_t cpu_mask, uint32_t timeout)
{
    ARG_UNUSED(timeout);
    uint32_t mask = cpu_mask;

    switch (mask) {
    case VCPU_IPI_MASK_ALL:
#if defined(CONFIG_SMP) && defined(CONFIG_SCHED_IPI_SUPPORTED)
		arch_sched_ipi();
#else
        ZVM_LOG_WARN("Not smp ipi support.");
#endif /* CONFIG_SMP && CONFIG_SCHED_IPI_SUPPORTED */
        break;
    default:
        break;
    }

    return 0;
}

/**
 * @brief vcpu run func entry_point.
 */
int z_vcpu_run(struct vcpu *vcpu)
{
    int ret = 0;

    do{
        ret = arch_vcpu_run(vcpu);
    }while(ret >= 0);

    vm_delete(vcpu->vm);

    return ret;
}

/**
 * @brief Judge whether this vcpu has irq need to handle...
 */
int vcpu_irq_exit(struct vcpu *vcpu)
{
    bool pend, active;
	struct virq_struct *vs = vcpu->virq_struct;

	pend = sys_dlist_is_empty(&vs->pending_irqs);
	active = sys_dlist_is_empty(&vs->active_irqs);

	return !(pend && active);
}

static int created_vm_num = 0;

struct vcpu *vm_vcpu_init(struct vm *vm, uint16_t vcpu_id, char *vcpu_name)
{
    uint16_t vm_prio;
    uint64_t tmp_addr;          /* For processing strange bug in init_vcpu_virt_irq_desc() */
    int pcpu_num;
    struct vcpu *vcpu;
    struct vcpu_work *vwork;

    vcpu = (struct vcpu *)k_malloc(sizeof(struct vcpu));
    if (!vcpu) {
        ZVM_LOG_ERR("Allocate vcpu space failed");
        return NULL;
    }

    vcpu->arch = (struct vcpu_arch *)k_malloc(sizeof(struct vcpu_arch));
    if (!vcpu->arch) {
        ZVM_LOG_ERR("Init vcpu->arch failed");
        k_free(vcpu);
        return  NULL;
    }

    vcpu->virq_struct = (struct virq_struct *)\
            k_malloc(sizeof(struct virq_struct));
    if (!vcpu->virq_struct) {
        ZVM_LOG_ERR("Init vcpu->virq_struct failed");
        k_free(vcpu);
        return  NULL;
    }
    vcpu->virq_struct->virq_act_counts = 0;
    sys_dlist_init(&vcpu->virq_struct->pending_irqs);
    sys_dlist_init(&vcpu->virq_struct->active_irqs);
    ZVM_SPINLOCK_INIT(&vcpu->virq_struct->spinlock);

    /* Bug: we must use tmp_addr to get this address. */
    tmp_addr = (uint64_t)vcpu->virq_struct;
    init_vcpu_virt_irq_desc(vcpu->virq_struct);
    vcpu->virq_struct = (struct virq_struct *)tmp_addr;

    if (vm->is_rtos) {
        vm_prio = VCPU_RT_PRIO;
    }
    else{
        vm_prio = VCPU_NORT_PRIO;
    }
    vcpu->vm = vm;

    /* vt_stack must be aligned, So we allocate memory with aligned block */
    vwork = (struct vcpu_work *)k_aligned_alloc(0x10, sizeof(struct vcpu_work));
    if (!vwork) {
        ZVM_LOG_ERR("Create vwork error!");
        return NULL;
    }

    /* init tast_vcpu_thread struct here */
    vwork->vcpu_thread = (struct k_thread *)k_malloc(sizeof(struct k_thread));
    if (!vwork->vcpu_thread) {
        ZVM_LOG_ERR("Init thread struct error here!");
        return  NULL;
    }
    /*TODO: In this stage, the thread is marked as a kernel thread,
    For system safe, we will modified it later.*/
    k_tid_t tid = k_thread_create(vwork->vcpu_thread, vwork->vt_stack,
            VCPU_THREAD_STACKSIZE,(void *)z_vcpu_run, vcpu, NULL, NULL,
			vm_prio, 0, K_FOREVER);
    strcpy(tid->name, vcpu_name);

    /* SMP support*/
#ifdef CONFIG_SCHED_CPU_MASK
    k_thread_cpu_mask_disable(tid, 0);

    if (vm->is_rtos) {
        pcpu_num = rt_get_idle_cpu();
    }else{
        pcpu_num = nrt_get_idle_cpu();
    }
    if (pcpu_num < 0 || pcpu_num >= CONFIG_MP_NUM_CPUS) {
        ZVM_LOG_WARN("No suitable idle cpu for VM! \n");
        return NULL;
    }
    /* Just work on 4 cores system */
    if(++created_vm_num == CONFIG_MP_NUM_CPUS-1){
        pcpu_num = CONFIG_MP_NUM_CPUS-1;
    }
    k_thread_cpu_mask_enable(tid, pcpu_num);
    vcpu->cpu = pcpu_num;
#endif /* CONFIG_SCHED_CPU_MASK */

    /* create a new thread and store it in work struct */
    vwork->v_date = vcpu;
    vwork->vcpu_thread->vcpu_struct = vcpu;

    vcpu->work = vwork;
    /* init vcpu timer*/
    vcpu->hcpu_cycles = 0;
    vcpu->runnig_cycles = 0;
    vcpu->paused_cycles = 0;
    vcpu->vcpu_id = vcpu_id;
    vcpu->vcpu_state = _VCPU_STATE_UNKNOWN;
    vcpu->resume_signal = false;
    vcpu->waitq_flag = false;

    if (arch_vcpu_init(vcpu)) {
        k_free(vcpu);
        return  NULL;
    }

    return vcpu;
}

int vm_vcpu_run(struct vcpu *vcpu)
{
    uint16_t cur_state = vcpu->vcpu_state;
    struct k_thread *thread;

    /*vcpu life time cycles*/
    vcpu->hcpu_cycles = sys_clock_cycle_get_32();
    thread = vcpu->work->vcpu_thread;

    switch (cur_state) {
    case _VCPU_STATE_READY:
    case _VCPU_STATE_UNKNOWN:
        k_thread_start(thread);
        vcpu->vcpu_state = _VCPU_STATE_READY;
        break;
    case _VCPU_STATE_PAUSED:
        vcpu_state_switch(thread, _VCPU_STATE_READY);
        break;
    default:
        /* Handle invalid state */
        ZVM_LOG_WARN("Unknow vcpu states! \n");
        break;
    }
    return 0;
}

int vm_vcpu_pause(struct vcpu *vcpu)
{
    return vcpu_state_switch(vcpu->work->vcpu_thread, _VCPU_STATE_PAUSED);
}

int vm_vcpu_halt(struct vcpu *vcpu)
{
    return vcpu_state_switch(vcpu->work->vcpu_thread, _VCPU_STATE_HALTED);
}
