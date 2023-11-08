/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <init.h>
#include <device.h>
#include <devicetree.h>
#include <virtualization/vdev/virt_device.h>


/**
 * A virt device sample that for zvm system.
 *
 * @brief Define the serial1 description for zvm, the sample:
 *
 * @param DT_ALIAS(serial1): node identifier for this device;
 *
 * @param &serial1_init: init function for zvm;
 *
 * @param NULL: pm_device, which usually not used;
 *
 * @param &serial1_data_port_1: serial config data, which is not hardware device related data;
 *
 * @param &virt_serial1_cfg: serial config data, which is hardware device related data;
 *
 * @param POST_KERNEL: init post kernel level;
 *
 * @param CONFIG_SERIAL_INIT_PRIORITY: init priority
 *
 * @param &serial_driver_api: serial driver api;
*/
/*
DEVICE_DT_DEFINE(DT_ALIAS(serial1),
            &serial_init,
            NULL,
            &serial1_data_port_1,
            &virt_serial1_cfg, POST_KERNEL,
            CONFIG_SERIAL_INIT_PRIORITY,
            &serial_driver_api);
*/
