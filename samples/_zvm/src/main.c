/*
 * Copyright (c) 2021 Huang Jiajia(Huangjj2020@hnu.edu.cn)
 * Copyright (c) 2022 xcl (xiongcl@hnu.edu.cn)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <sys/printk.h>
#include <sys/mem_manage.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <drivers/uart.h>
#include <kernel/thread.h>
#include <timing/timing.h>
#include <sys/time_units.h>


/* For test time slice vm change */
int main(int argc, char **argv)
{

	printk("--Init zvm successful! --\n");
	printk("--Ready to input shell cmd to create and build vm!--\n");

	return 0;
}
