/*
 * Copyright 2021-2022 HNU-ESNL
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/mem_manage.h>
#include <virtualization/zvm.h>
#include <virtualization/vm_manager.h>
#include <virtualization/os/os_linux.h>


static atomic_t zvm_linux_image_map_init = ATOMIC_INIT(0);
static uint64_t zvm_linux_image_map_phys = 0;

/**
 * @brief Establish a mapping between the linux image physical
 * addresses and virtual addresses.
 */
static uint64_t zvm_mapped_linux_image(void)
{
    uint8_t *ptr;
    uintptr_t phys;
    size_t size;
    uint32_t flags;
    if(likely(!atomic_cas(&zvm_linux_image_map_init, 0, 1))){
        return zvm_linux_image_map_phys;
    }

    phys = LINUX_VM_IMAGE_BASE;
    size = LINUX_VM_IMAGE_SIZE;
    flags = K_MEM_CACHE_NONE | K_MEM_PERM_RW | K_MEM_PERM_EXEC;
    z_phys_map(&ptr, phys, size, flags);
    zvm_linux_image_map_phys = (uint64_t)ptr;
    return zvm_linux_image_map_phys;
}

int load_linux_image(struct vm_mem_domain *vmem_domain)
{
    int ret = 0;
    uint64_t lbase_size, limage_base, limage_size;
    struct _dnode *d_node, *ds_node;
    struct vm_mem_partition *vpart;
    uint64_t *src_hva, des_hva;
    uint64_t num_m = LINUX_VM_IMAGE_SIZE / (1024 * 1024);
    uint64_t src_hpa = LINUX_VMCPY_BASE;
    uint64_t des_hpa = LINUX_VM_IMAGE_BASE;
    uint64_t per_size = 1048576; //1M

    ZVM_LOG_INFO("1 image_num_m = %ld\n", num_m);
    ZVM_LOG_INFO("1 image_src_hpa = 0x%lx\n", src_hpa);
    ZVM_LOG_INFO("1 image_des_hpa = 0x%lx\n", des_hpa);

    while(num_m){
        z_phys_map((uint8_t **)&src_hva, (uintptr_t)src_hpa, per_size, K_MEM_CACHE_NONE | K_MEM_PERM_RW);
        z_phys_map((uint8_t **)&des_hva, (uintptr_t)des_hpa, per_size, K_MEM_CACHE_NONE | K_MEM_PERM_RW);
        memcpy(des_hva, src_hva, per_size);
        des_hpa += per_size;
        src_hpa += per_size;
        num_m--;
    }

    num_m = LINUX_VMRFS_SIZE / (1024 * 1024);
    src_hpa = LINUX_VMRFS_BASE;
    des_hpa = LINUX_VMRFS_PHY_BASE;

    ZVM_LOG_INFO("1 rf_num_m = %ld\n", num_m);
    ZVM_LOG_INFO("1 rf_src_hpa = 0x%lx\n", src_hpa);
    ZVM_LOG_INFO("1 rf_des_hpa = 0x%lx\n", des_hpa);

    while(num_m){
        z_phys_map((uint8_t **)&src_hva, (uintptr_t)src_hpa, per_size, K_MEM_CACHE_NONE | K_MEM_PERM_RW);
        z_phys_map((uint8_t **)&des_hva, (uintptr_t)des_hpa, per_size, K_MEM_CACHE_NONE | K_MEM_PERM_RW);
        memcpy(des_hva, src_hva, per_size);
        des_hpa += per_size;
        src_hpa += per_size;
        num_m--;
    }

#ifndef  CONFIG_VM_DYNAMIC_MEMORY
    return ret;
#endif /* CONFIG_VM_DYNAMIC_MEMORY */

    lbase_size =  LINUX_VMSYS_SIZE;
    limage_base = zvm_mapped_linux_image();
    limage_size = LINUX_VM_IMAGE_SIZE;
    SYS_DLIST_FOR_EACH_NODE_SAFE(&vmem_domain->mapped_vpart_list,d_node,ds_node){
        vpart = CONTAINER_OF(d_node,struct vm_mem_partition,vpart_node);
        if(vpart->part_hpa_size == lbase_size){
            memcpy((void *)vpart->part_hpa_base, (const void *)limage_base, limage_size);
            break;
        }
    }

    return ret;
}
