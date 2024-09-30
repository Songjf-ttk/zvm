/*
 * Copyright 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PSCI_PSCI_H_
#define ZEPHYR_DRIVERS_PSCI_PSCI_H_

#include <drivers/pm_cpu_ops/psci.h>

#ifdef CONFIG_64BIT
#define PSCI_FN_NATIVE(version, name)		PSCI_##version##_FN64_##name
#else
#define PSCI_FN_NATIVE(version, name)		PSCI_##version##_FN_##name
#endif

typedef unsigned long (psci_fn)(unsigned long, unsigned long,
				unsigned long, unsigned long);

struct psci {
	enum arm_smccc_conduit conduit;
	psci_fn *invoke_psci_fn;
	uint32_t ver;
};

#endif /* ZEPHYR_DRIVERS_PSCI_PSCI_H_ */
