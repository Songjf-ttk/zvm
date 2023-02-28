/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <arch/cpu.h>
#include <arch/arm64/lib_helpers.h>

void main(void){
    uint32_t curr_el = GET_EL(read_currentel());

    printk("Hello world, It is a %s board. \r\n", CONFIG_BOARD);
    printk("Current EL = %d.\n", curr_el);

}
