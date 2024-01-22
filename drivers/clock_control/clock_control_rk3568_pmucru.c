/*
 * Copyright 2023 HNU-ESNL
 * Copyright 2023 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */
//need edit not for vm system first.

#include <errno.h>
#include <soc.h>
#include <sys/util.h>
#include <drivers/clock_control.h>
#include <dt-bindings/clock/rk3568_clock.h>

#include <virtualization/vdev/virt_device.h>

static int rk3568_pmucru_clk_init(const struct device *dev)
{
	dev->state->init_res = VM_DEVICE_INIT_RES;
	return 0;
}

static int rk3568_pmucru_clk_on(const struct device *dev,
			      clock_control_subsys_t sub_system)
{
	return 0;
}

static int rk3568_pmucru_clk_off(const struct device *dev,
			       clock_control_subsys_t sub_system)
{
	return 0;
}

static const struct clock_control_driver_api rk3568_pmucru_driver_api = {
	.on = rk3568_pmucru_clk_on,
	.off = rk3568_pmucru_clk_off,
};

static int vm_pmucru_init(const struct device *dev, struct vm *vm, struct virt_dev *vdev_desc)
{
	return 0;
}

static const struct virt_device_api virt_pmucru_api = {
	.init_fn = vm_pmucru_init,
	.device_driver_api = &rk3568_pmucru_driver_api,
};

static struct virt_device_config virt_pmucru_cfg = {
	.reg_base = DT_REG_ADDR(DT_NODELABEL(pmucru)),
	.reg_size = DT_REG_SIZE(DT_NODELABEL(pmucru)),
	.hirq_num = VM_DEVICE_INVALID_VIRQ,
};

DEVICE_DT_DEFINE(DT_NODELABEL(pmucru),
		    &rk3568_pmucru_clk_init,
		    NULL,
		    NULL, &virt_pmucru_cfg,
		    PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &virt_pmucru_api);
