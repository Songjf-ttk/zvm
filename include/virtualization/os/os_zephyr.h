/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZVM_OS_ZEPHYR_H_
#define __ZVM_OS_ZEPHYR_H_

#include <zephyr.h>
#include <stdint.h>
#include <virtualization/arm/mm.h>

/**
 * ZEPHYR_VM_IMAGE_BASE presents that the zephyr image base
 * in the ddr(@TODO: In the disk better). And ZEPHYR_VM_IMAGE_SIZE
 * presents the zephyr image size in the ddr.
*/
#define ZEPHYR_VM_IMAGE_BASE    DT_REG_ADDR(DT_NODELABEL(zephyr_ddr))
#define ZEPHYR_VM_IMAGE_SIZE    DT_REG_SIZE(DT_NODELABEL(zephyr_ddr))
#define ZEPHYR_VMSYS_BASE       DT_PROP(DT_NODELABEL(zephyr_ddr), vm_reg_base)
#define ZEPHYR_VMSYS_SIZE       DT_PROP(DT_NODELABEL(zephyr_ddr), vm_reg_size)


int load_zephyr_image(struct vm_mem_domain *vmem_domain);

#endif /* __ZVM_OS_ZEPHYR_H_ */
