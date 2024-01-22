/*
 * Copyright (c) Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT syscon

#include <sys/util.h>
#include <device.h>
#include <init.h>

#include <drivers/syscon.h>
#include <virtualization/vdev/virt_device.h>
#include "syscon_common.h"

struct syscon_generic_config {
	DEVICE_MMIO_ROM;
	uint8_t reg_width;
};

struct syscon_generic_data {
	DEVICE_MMIO_RAM;
	size_t size;
};

static int syscon_generic_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
#ifdef CONFIG_SOC_RK3568
	/*Set flag as the idle device which can bind to vm */
	dev->state->init_res = VM_DEVICE_INIT_RES;
#endif
	return 0;
}

static int syscon_generic_get_base(const struct device *dev, uintptr_t *addr)
{
	if (!dev) {
		return -ENODEV;
	}

	*addr = DEVICE_MMIO_GET(dev);
	return 0;
}

static int syscon_generic_read_reg(const struct device *dev, uint16_t reg, uint32_t *val)
{
	const struct syscon_generic_config *config;
	struct syscon_generic_data *data;
	uintptr_t base_address;

	if (!dev) {
		return -ENODEV;
	}

	data = dev->data;
	config = dev->config;

	if (!val) {
		return -EINVAL;
	}

	if (syscon_sanitize_reg(&reg, data->size, config->reg_width)) {
		return -EINVAL;
	}

	base_address = DEVICE_MMIO_GET(dev);

	switch (config->reg_width) {
	case 1:
		*val = sys_read8(base_address + reg);
		break;
	case 2:
		*val = sys_read16(base_address + reg);
		break;
	case 4:
		*val = sys_read32(base_address + reg);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int syscon_generic_write_reg(const struct device *dev, uint16_t reg, uint32_t val)
{
	const struct syscon_generic_config *config;
	struct syscon_generic_data *data;
	uintptr_t base_address;

	if (!dev) {
		return -ENODEV;
	}

	data = dev->data;
	config = dev->config;

	if (syscon_sanitize_reg(&reg, data->size, config->reg_width)) {
		return -EINVAL;
	}

	base_address = DEVICE_MMIO_GET(dev);

	switch (config->reg_width) {
	case 1:
		sys_write8(val, (base_address + reg));
		break;
	case 2:
		sys_write16(val, (base_address + reg));
		break;
	case 4:
		sys_write32(val, (base_address + reg));
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int syscon_generic_get_size(const struct device *dev, size_t *size)
{
	struct syscon_generic_data *data = dev->data;

	*size = data->size;
	return 0;
}

static const struct syscon_driver_api syscon_generic_driver_api = {
	.read = syscon_generic_read_reg,
	.write = syscon_generic_write_reg,
	.get_base = syscon_generic_get_base,
	.get_size = syscon_generic_get_size,
};

static int vm_syscon_init(const struct device *dev, struct vm *vm, struct virt_dev *vdev_desc)
{
	return 0;
}

static const struct virt_device_api virt_syscon_api = {
	.init_fn = vm_syscon_init,
	.device_driver_api = &syscon_generic_driver_api,
};

#define SYSCON_INIT(inst)                                                                  \
	static const struct virt_device_config syscon_generic_config_##inst = {                 \
		.reg_base = DT_INST_REG_ADDR(inst),                          \
		.reg_size = DT_INST_REG_SIZE(inst),                          \
		.hirq_num = VM_DEVICE_INVALID_VIRQ,                          \
	};                                                                                   \
	static struct syscon_generic_data syscon_generic_data_##inst = {                      \
		.size = DT_INST_REG_SIZE(inst),                                                    \
	};                                                                                      \
	DEVICE_DT_INST_DEFINE(inst, syscon_generic_init, NULL, &syscon_generic_data_##inst,      \
			      &syscon_generic_config_##inst, PRE_KERNEL_1,                         \
			      CONFIG_SYSCON_INIT_PRIORITY, &virt_syscon_api);

DT_INST_FOREACH_STATUS_OKAY(SYSCON_INIT);
