/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/mem_manage.h>

#include <virtualization/os/os.h>
#include <virtualization/zvm.h>
#include <virtualization/arm/mm.h>
#include <virtualization/os/os_zephyr.h>
#include <virtualization/vm_mm.h>

static atomic_t zvm_zephyr_image_map_init = ATOMIC_INIT(0);
static uint64_t zvm_zephyr_image_map_phys = 0;


/**
 * @brief Establish a mapping between the zephyr image physical
 * addresses and virtual addresses.
 */
static uint64_t zvm_mapped_zephyr_image(void)
{
    uint8_t *ptr;
    uintptr_t phys;
    size_t size;
    uint32_t flags;
    if(likely(!atomic_cas(&zvm_zephyr_image_map_init,0,1))){
        return zvm_zephyr_image_map_phys;
    }

    phys = ZEPHYR_VM_IMAGE_BASE;
    size = ZEPHYR_VM_IMAGE_SIZE;
    flags = K_MEM_CACHE_NONE | K_MEM_PERM_RW | K_MEM_PERM_EXEC;
    z_phys_map(&ptr,phys,size,flags);
    zvm_zephyr_image_map_phys = (uint64_t)ptr;
    return zvm_zephyr_image_map_phys;
}

int load_zephyr_image(struct vm_mem_domain *vmem_domain)
{
    char *dbuf, *sbuf;
    ARG_UNUSED(dbuf);
    ARG_UNUSED(sbuf);
    int ret = 0;
    uint64_t zbase_size,zimage_base,zimage_size;
    struct _dnode *d_node, *ds_node;
    struct vm_mem_partition *vpart;
    struct vm_mem_block *blk;
    struct vm *this_vm = vmem_domain->vm;
    ARG_UNUSED(blk);
    ARG_UNUSED(this_vm);

#ifndef  CONFIG_VM_DYNAMIC_MEMORY
    ARG_UNUSED(this_vm);
    return ret;
#endif /* CONFIG_VM_DYNAMIC_MEMORY */

    zbase_size = ZEPHYR_VMSYS_SIZE;
    zimage_base = zvm_mapped_zephyr_image();
    zimage_size = ZEPHYR_VM_IMAGE_SIZE;
    SYS_DLIST_FOR_EACH_NODE_SAFE(&vmem_domain->mapped_vpart_list,d_node,ds_node){
        vpart = CONTAINER_OF(d_node,struct vm_mem_partition,vpart_node);
        if(vpart->part_hpa_size == zbase_size){
            memcpy((void *)vpart->part_hpa_base,(const void *)zimage_base,zimage_size);
            break;
        }
    }

    return ret;
}
