/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZVM_OS_ZEPHYR_H_
#define __ZVM_OS_ZEPHYR_H_

#include <zephyr.h>
#include <stdint.h>

#include <_zvm/arm/mm.h>



/* Get vm image info from dts file */
#define ZEPHYR_VM_MEM_BASE      DT_REG_ADDR(DT_PATH(vm_zephyr_space, ZEPHYR_VMMM_STRING))
#define ZEPHYR_VM_MEM_SIZE      DT_REG_SIZE(DT_PATH(vm_zephyr_space, ZEPHYR_VMMM_STRING))
#define ZEPHYR_VMSYS_ENTRY      DT_PROP(DT_PATH(vm_zephyr_space, ZEPHYR_VMMM_STRING), vm_reg)
#define ZEPHYR_VM_IMG_SIZE      DT_PROP(DT_PATH(vm_zephyr_space, ZEPHYR_VMMM_STRING), img_sz)

/**
 * @brief load zephyr image from other memory address to allocated address
 */
int load_zephyr_image(struct vm_mem_domain *vmem_domain);



#endif /* __ZVM_OS_ZEPHYR_H_ */