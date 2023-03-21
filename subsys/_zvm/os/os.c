/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr.h>

#include <_zvm/os/os.h>
#include <_zvm/zvm.h>
#include <_zvm/tools/elfloader.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

/* find linux memory address's base and size */
static int get_vm_mem_info(uint32_t *base, uint32_t *size, uint64_t virt_base)
{
    switch (virt_base){
    case LINUX_VMSYS_ENTRY:
        *base = LINUX_VM_MEM_BASE;
        *size = LINUX_VM_MEM_SIZE;
        break;
    default:
        break;
    }

    return 0;
}


int vm_os_create(struct vm* vm, struct z_vm_info *vm_info)
{
    struct os* os = vm->os;
    os->type = vm_info->vm_os_type;
    os->name = k_malloc(sizeof(char)*OS_NAME_LENGTH);
    memset(os->name, '\0', OS_NAME_LENGTH);

    switch (os->type){
    case OS_TYPE_LINUX:
        strcpy(os->name, "linux_os");
        vm->is_rtos = false;
        break;
    case OS_TYPE_ZEPHYR:
        strcpy(os->name, "zephyr_os");
        vm->is_rtos = true;
        break;
    default:
        return -EMMAO;
        break;
    }
    os->vm_virt_base = vm_info->vm_virt_base;
    os->code_entry_point = vm_info->entry_point;
    os->os_mem_base = vm_info->vm_image_base;
    os->os_mem_size = vm_info->vm_image_size;

    return 0;

}

char *strlwr(char *s){
    char *str;
    str = s;

    while (*str != '\0'){
        if (*str >= 'A' && *str <= 'Z') {
            *str += 'a'-'A';
        }
        str++;
    }

    return s;
}

uint32_t get_os_id_by_type(char *s){
    uint32_t os_type;
    char *lower = strlwr(s);

    if (strcmp(lower, "linux") == 0){
        os_type = OS_TYPE_LINUX;
    }else if (strcmp(lower, "zephyr") == 0){
        os_type = OS_TYPE_ZEPHYR;
    }else{
        os_type = OS_TYPE_OTHERS;
    }

    return os_type;
}

char *get_os_type_by_id(uint32_t osid){
    switch (osid){
    case OS_TYPE_LINUX:
        return "linux";
        break;
    case OS_TYPE_ZEPHYR:
        return "zephyr";
        break;
    default:
        return "others";
        break;
    }
}

extern z_vm_info_t z_overall_vm_infos[];


/**
 * @brief Get the os info by vm id object.
 *
 */
int get_os_info_by_id(struct getopt_state *state, struct z_vm_info *vm_info)
{
    int ret = 0;
    uint16_t vm_id = 0 ;
    struct z_vm_info tmp_vm_info;

    switch ((state->optarg)[0]){
    /* zephyr os */
    case '0':
        vm_id = OS_TYPE_ZEPHYR;
        tmp_vm_info = z_overall_vm_infos[vm_id];
        break;
    /* linux os */
    case '1':
        vm_id = OS_TYPE_LINUX;
        tmp_vm_info = z_overall_vm_infos[vm_id];
        break;
    /* other os is not supported */
    default:
        ZVM_LOG_WARN("The VM is not default vm, not supported here!");
        ret = -1;
        return ret;
    }
	vm_info->vcpu_num = tmp_vm_info.vcpu_num;
	vm_info->vm_image_size = tmp_vm_info.vm_image_size;
    vm_info->vm_image_base = tmp_vm_info.vm_image_base;
    vm_info->vm_virt_base = tmp_vm_info.vm_virt_base;
	vm_info->vm_os_type = tmp_vm_info.vm_os_type;

    /* Get the vm's entry point */
#if defined(CONFIG_SOC_QEMU_CORTEX_MAX)

#ifdef  CONFIG_ZVM_ELF_LOADER
    ret = elf_loader((void *)tmp_vm_info.vm_image_base, NULL, vm_info);
#else
    vm_info->entry_point = tmp_vm_info.entry_point;
#endif  /* CONFIG_ZVM_ELFLOADER */

#else
    vm_info->entry_point = tmp_vm_info.entry_point;
#endif  /* CONFIG_SOC_QEMU_CORTEX_MAX */

    return ret;
}

/**
 * @brief Get the os info by vm id object.
 */
int get_os_info_by_type(struct getopt_state *state, struct z_vm_info *vm_info)
{
    char *vm_type = state->optarg;
    int ret = 0;
    struct z_vm_info tmp_vm_info;

    if (strcmp(vm_type, "zephyr") == 0){
        tmp_vm_info = z_overall_vm_infos[OS_TYPE_ZEPHYR];
        goto out;
    }

    if (strcmp(vm_type, "linux") == 0){
        tmp_vm_info = z_overall_vm_infos[OS_TYPE_LINUX];
        goto out;
    }

    ZVM_LOG_WARN("The VM type is not supported(Linux or zephyr). \n Please try again! \n");
    return -EINVAL;

out:
	vm_info->vcpu_num = tmp_vm_info.vcpu_num;
    vm_info->vm_image_base = tmp_vm_info.vm_image_base;
	vm_info->vm_image_size = tmp_vm_info.vm_image_size;
    vm_info->vm_virt_base = tmp_vm_info.vm_virt_base;
	vm_info->vm_os_type = tmp_vm_info.vm_os_type;

    /* Get the vm's entry point */
#if defined(CONFIG_SOC_QEMU_CORTEX_MAX)

#ifdef  CONFIG_ZVM_ELF_LOADER
    ret = elf_loader((void *)tmp_vm_info.vm_image_base, NULL, vm_info);
#else
    vm_info->entry_point = tmp_vm_info.entry_point;
#endif  /* CONFIG_ZVM_ELFLOADER */

#else
    vm_info->entry_point = tmp_vm_info.entry_point;
#endif  /* CONFIG_SOC_QEMU_CORTEX_MAX */

    return ret;
}


/**
 * @brief Get the os image info by type object.
 */
int get_vm_mem_info_by_type(uint32_t *base, uint32_t *size,
            uint16_t type, uint64_t virt_base)
{
    switch (type){
    case OS_TYPE_LINUX:
        get_vm_mem_info(base, size, virt_base);
        break;
    case OS_TYPE_ZEPHYR:
        base[0] = ZEPHYR_VM_MEM_BASE;
        size[0] = ZEPHYR_VM_MEM_SIZE;
        break;
    default:
        return -EVMOS;
        break;
	}
    return 0;
}
