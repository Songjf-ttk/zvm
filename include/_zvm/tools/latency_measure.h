/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LANTENCY_MEASURE_H_
#define __LANTENCY_MEASURE_H_

#include <zephyr.h>


/* call this function when irq exit */
int vm_exit_irq_time(void);

/* call this function when irq entry */
int vm_entry_irq_time(void);

/*print vm irq timing measure value*/
void vm_irq_timing_print(void);

#endif /* __LANTENCY_MEASURE_H_ */
