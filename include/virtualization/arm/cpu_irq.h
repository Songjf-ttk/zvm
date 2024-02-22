/*
 * Copyright 2022-2023 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_CPU_IRQ_H_
#define ZEPHYR_INCLUDE_ZVM_ARM_CPU_IRQ_H_

#include <virtualization/vm.h>

bool arch_irq_ispending(struct vcpu *vcpu);

#endif /* ZEPHYR_INCLUDE_ZVM_ARM_CPU_IRQ_H_ */