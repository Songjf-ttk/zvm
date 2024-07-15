/*
 * Copyright 2023 HNU-ESNL
 * Copyright 2023 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/util.h>
#include <device.h>
#include <init.h>
#include <drivers/syscon.h>
#include <virtualization/zvm.h>
#include <virtualization/vm_device.h>

#define DT_DRV_COMPAT syscon

void rk3568_syscon_generic_init(const struct device *dev)
{
	/*Set flag as the idle device which can bind to vm */
	dev->state->init_res = VM_DEVICE_INIT_RES;
}

#define SYSCON_INIT(inst)    \
	static const struct virt_device_config virt_syscon_generic_config_##inst = {          \
		.reg_base = DT_INST_REG_ADDR(inst),                          \
		.reg_size = DT_INST_REG_SIZE(inst),                          \
		.hirq_num = VM_DEVICE_INVALID_VIRQ,                          \
	};                                                               \                                                                                      \
	DEVICE_DT_INST_DEFINE(inst, rk3568_syscon_generic_init, NULL, NULL,           \
			      &virt_syscon_generic_config_##inst, PRE_KERNEL_1,               \
			      CONFIG_SYSCON_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(SYSCON_INIT);
