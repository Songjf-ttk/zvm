/*
 * Copyright 2021-2022 HNU-ESNL
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
#include <virtualization/zvm.h>

/* For test time slice vm change */
int main(int argc, char **argv)
{
	ZVM_LOG_INFO("--Init zvm successful! --\n");
	ZVM_LOG_INFO("--Ready to input shell cmd to create and build vm!--\n");

	return 0;
}
