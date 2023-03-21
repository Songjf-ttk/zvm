/*
 * Copyright 2021-2022 HNU
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZVM_VIRT_OS_H_
#define __ZVM_VIRT_OS_H_

#include <zephyr.h>
#include <stdint.h>

#include <_zvm/os/os_zephyr.h>
#include <_zvm/os/os_linux.h>

struct getopt_state;

#define OS_NAME_LENGTH 32

/* Default vcpu num is 1 */
#define VM_DEFAULT_VCPU_NUM     (1)


#ifdef CONFIG_SOC_FVP_CORTEX_A55

#define ZEPHYR_VMMM_STRING  memory_a0000000
#define LINUX_VMMM_STRING   memory_b0000000

#elif CONFIG_SOC_QEMU_CORTEX_MAX

#define ZEPHYR_VMMM_STRING  memory_48000000
#define LINUX_VMMM_STRING   memory_80000000

#endif /* CONFIG_SOC_FVP_CORTEX_A55 */


/* For clear warning for unknow reason */
struct z_vm_info;
struct vm;

enum {
    OS_TYPE_ZEPHYR = 0,
    OS_TYPE_LINUX  = 1,
    OS_TYPE_OTHERS = 2,
    OS_TYPE_MAX = 3,
};

/**
 * @brief A descriptor of os
 *
 */
struct os {
    char *name;
    uint16_t type;

    /* memory area num */
    uint16_t area_num;
    uint64_t vm_virt_base;

    /* vm image's code entry */
    uint64_t code_entry_point;

    /* vm image's base address */
    uint64_t os_mem_base;

    /* vm image's size */
    uint64_t os_mem_size;
};

/**
 * @brief init os type and name, for some purpose
 *
 * @param os_type
 */
int vm_os_create(struct vm *vm, struct z_vm_info *vm_info);

/* Return lower string. */
char *strlwr(char *s);

uint32_t get_os_id_by_type(char *s);

char *get_os_type_by_id(uint32_t osid);


/**
 * @brief Get the os info by vm id object
 * The meanning id for the system:
 *
 * 0 - zephyr os
 * 1 - linux os
 *
 */
int get_os_info_by_id(struct getopt_state *state, struct z_vm_info *vm_info);

/**
 * @brief Get the os info by vm type object
 * The meanning id for the system:
 *
 * "zephyr" - zephyr os
 * "linux"  - linux os
 *
 */
int get_os_info_by_type(struct getopt_state *state, struct z_vm_info *vm_info);

/**
 * @brief Get the os image info by type object.
 * Get the os's image info from dts file.
 * @param base
 * @param size
 * @param type
 * @return int
 */
int get_vm_mem_info_by_type(uint32_t *base, uint32_t *size, uint16_t type, uint64_t virt_base);



#endif  /* __ZVM_VIRT_OS_H_ */
