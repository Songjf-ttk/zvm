// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 Rockchip Electronics Co. Ltd.
 * Author: Elaine Zhang <zhangqing@rock-chips.com>
 */
//need edit
#define DT_DRV_COMPAT rockchip_rk3568_cru

#include <errno.h>
#include <soc.h>
#include <sys/util.h>
#include <drivers/clock_control.h>
#include <dt-bindings/clock/rk3568-cru.h>
#include <fsl_clock.h>
#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(clock_control);



static int rk3568_clk_on(const struct device *dev,
			      clock_control_subsys_t sub_system)
{
	return 0;
}

static int rk3568_clk_off(const struct device *dev,
			       clock_control_subsys_t sub_system)
{
	return 0;
}

static int rk3568_clk_init(const struct device *dev)
{
	return 0;
}

static const struct clock_control_driver_api rk3568_clk_driver_api = {
	.on = rk3568_clk_on,
	.off = rk3568_clk_off,
};

DEVICE_DT_INST_DEFINE(0,
		    &rk3568_clk_init,
		    NULL,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &rk3568_clk_driver_api);
