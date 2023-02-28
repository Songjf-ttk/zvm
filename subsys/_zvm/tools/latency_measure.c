/*
 * Copyright (c) 2023 xcl (xiongcl@hnu.edu.cn)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <timing/timing.h>

#define TICK_SYNCH()  k_sleep(K_TICKS(1))

static volatile int flag_var;

static timing_t timestamp_start;
static timing_t timestamp_end;

static struct k_spinlock time_start_lock;
static struct k_spinlock time_end_lock;

static void vm_timing_print(uint32_t diff)
{
    printk("Cycles: %d \t ns: %d \n", diff, (uint32_t)timing_cycles_to_ns(diff));
}

/* call this function when irq exit */
int vm_exit_irq_time(void)
{
    k_spinlock_key_t key;
    key = k_spin_lock(&time_start_lock);
    timestamp_start = timing_counter_get();
    k_spin_unlock(&time_start_lock, key);
}

/* call this function when irq entry */
int vm_entry_irq_time(void)
{
    k_spinlock_key_t key;
    key = k_spin_lock(&time_end_lock);
    timestamp_start = timing_counter_get();
    k_spin_unlock(&time_end_lock, key);
}


/*print vm irq timing measure value*/
void vm_irq_timing_print(void)
{
    uint32_t diff = 0;
    k_spinlock_key_t key;
    key = k_spin_lock(&time_start_lock);
    diff = timing_cycles_get(&timestamp_start, &timestamp_end);
    k_spin_unlock(&time_start_lock, key);

    printk("The IRQ switch laytency: \n");
    vm_timing_print(diff);
}

/**
 * @brief The test main function
 *
 * @return 0 on success
 */
int int_to_vm_thread(void)
{
	uint32_t diff;

	timing_start();
	TICK_SYNCH();
	make_int();
	if (flag_var == 1) {
		diff = timing_cycles_get(&timestamp_start, &timestamp_end);
		PRINT_STATS("Switch from ISR back to interrupted thread", diff);
	}
	timing_stop();
	return 0;
}


/**
 * @brief init measure timing.
 */
void zvm_init_mtiming(void)
{
    timing_init();
}
