/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VM_MM_H_
#define ZEPHYR_INCLUDE_ZVM_VM_MM_H_

#include <zephyr.h>
#include <kernel.h>
#include <stdint.h>
#include <app_memory/mem_domain.h>

#include <virtualization/zvm.h>
#include <virtualization/arm/mm.h>

/* Memory size for block */
#define BLK_MAP         0x01000000
#define PGE_MAP         0x02000000

/* Memory block size: 64K */
#define DEFAULT_BLK_MEM_SHIFT       (12)
#define ZEPHYR_BLK_MEM_SHIFT        (16)
#define LINUX_BLK_MEM_SHIFT         (21)

#define DEFAULT_VM_BLOCK_SIZE   (1UL << DEFAULT_BLK_MEM_SHIFT)   //4K
#define ZEPHYR_VM_BLOCK_SIZE    (1UL << ZEPHYR_BLK_MEM_SHIFT)    //64K
#define LINUX_VM_BLOCK_SIZE     (1UL << LINUX_BLK_MEM_SHIFT)     //2M

/* The max vmid is CONFIG_MAX_VM_NUM 
 * Zephyr's ID is placed in the first half 
 * Linux's ID is placed in the second half 
 */
#define ZVM_ZEPHYR_VM_NUM (CONFIG_MAX_VM_NUM/2 + CONFIG_MAX_VM_NUM%2)
#define ZVM_LINUX_VM_NUM  (CONFIG_MAX_VM_NUM/2)


/* For clear warning for unknow reason */
struct vm;

/**
 * @brief vm_mem_block record the translation relation of virt addr to phy addr
 */
struct vm_mem_block{
    uint8_t     *phy_pointer;

    /* block num of this vpart */
    uint64_t    cur_blk_offset;

    uint64_t    phy_base;
    uint64_t    virt_base;

    /* block list of this vpart */
    sys_dnode_t vblk_node;
};

/**
 * @brief vwork_mm_area store one of VM task area
 */
struct vm_mem_partition {

    /* memory partition of vm*/
    struct k_mem_partition *vm_mm_partition;

    uint64_t part_hpa_base;
    uint64_t part_hpa_size;

    /* the vm_mem_partition node link to vm mm */
    sys_dnode_t vpart_node;

    /* mem_block lists for physical memmory management */
    sys_dlist_t blk_list;

    /* vwork_mm_area belong to one vmem_domain */
    struct  vm_mem_domain   *vmem_domain;
    
#ifdef CONFIG_VM_DYNAMIC_MEMORY
    /* base address of memory allocated by kmalloc */
    uint64_t part_kpa_base;
#endif 
};

/**
 * @brief vm_mem_domain describe the full virtual address space of the vm.
 */
struct vm_mem_domain{
    bool is_init;

    /* A vm is bind to a domain */
    struct k_mem_domain *vm_mm_domain;
    uint64_t    pgd_addr;

    /* vm_mem_partition list for mapped and idle list, some dev that used will
        in mapped list, otherwise in idle list.  */
    sys_dlist_t idle_vpart_list;
    sys_dlist_t mapped_vpart_list;

    struct k_spinlock spin_mmlock;
    struct vm *vm;
};

/**
 * @brief Map virtual addr 'vpart' to physical addr 'phy'.
 * this function aim to build the page table for virt addr to phys addr.
 *
 * @param vmem_domain : vm's mm struct
 * @param vpart : vpart for each task
 * @return int : 0--success, other for error code.
 */
int map_vpart_to_block(struct vm_mem_domain *vmem_domain, struct vm_mem_partition *vpart, uint64_t unit_msize);
int unmap_vpart_to_block(struct vm_mem_domain *vmem_domain, struct vm_mem_partition *vpart);

/**
 * @brief Create the vdev memory partition.
 */
int vm_vdev_mem_create(struct vm_mem_domain *vmem_domain, uint64_t hpbase,
                    uint64_t ipbase, uint64_t size, uint32_t attrs);

/**
 * @brief Init vm mm struct for this vm.
 * This function init the vm_mm struct for vm. Including init vpart list and
 * set the origin virtual space for vm, call  alloc_vm_mem_partition func to init
 * specific vpart struct and add it to unused vpart list.
 * 1.Set the total vm address space for this vm.
 * 2.Add it to the origin vpart space.
 *
 * @param vm : vm struct for store vm_mm struct
 */
int vm_mem_domain_create(struct vm *vm);

/**
 * @brief add mm area to this vm's mm space.
 * This function do not used only by vm init function, if we want to
 * add memory area for this vm, we should use it too. And it will allocate
 * a memory area for the user, and add it to used vpart list and ready to memory map.
 *
 * @param vmem_dm: the vm's mm space struct.
 *
 * @return int: error code.
 */
int vm_dynmem_apart_add(struct vm_mem_domain *vmem_dm);

/* Add area partition to vm memory struct */
int vm_mem_apart_add(struct vm_mem_domain *vmem_dm);

/* Remove area partition from the vm memory struct */
int vm_mem_apart_remove(struct vm_mem_domain *vmem_dm);

/**
 * @brief init vm's domain
 */
int arch_vm_mem_domain_init(struct k_mem_domain *domain, uint32_t vmid);

#endif /* ZEPHYR_INCLUDE_ZVM_VM_MM_H_ */
