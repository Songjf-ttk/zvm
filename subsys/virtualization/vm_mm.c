/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/mem_manage.h>
#include <sys/dlist.h>
#include <sys/util.h>

#include <virtualization/zvm.h>
#include <virtualization/arm/mm.h>
#include <virtualization/os/os.h>
#include <virtualization/os/os_zephyr.h>
#include <virtualization/os/os_linux.h>
#include <virtualization/vm.h>
#include <virtualization/vm_mm.h>
#include "../../../arch/arm64/core/mmu.h"

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);


struct k_spinlock vm_mem_domain_lock;
static uint8_t vm_max_partitions = CONFIG_MAX_DOMAIN_PARTITIONS;
static struct k_spinlock z_vm_domain_lock;


/**
 * @brief add vpart_space to vm's unused list area.
 */
static int add_idle_vpart(struct vm_mem_domain *vmem_domain,
            struct vm_mem_partition *vpart)
{

    /* add vpart free list to mem_domain unused list */
    if (vpart->vpart_node.next == NULL) {
        sys_dlist_append(&vmem_domain->idle_vpart_list, &vpart->vpart_node);
    }

    return 0;
}

/**
 * @brief add vpart_space to vm's used list area.
 */
int add_mapped_vpart(struct vm_mem_domain *vmem_domain,
            struct vm_mem_partition *vpart)
{
    if (!vpart) {
        return -1;
    }

    sys_dlist_append(&vmem_domain->mapped_vpart_list, &vpart->vpart_node);
    return 0;
}

static struct vm_mem_partition *alloc_vm_mem_partition(uint64_t hpbase,
            uint64_t ipbase, uint64_t size, uint32_t attrs)
{
    struct vm_mem_partition *vpart;

    /* allocate memory for vpart */
    vpart = (struct vm_mem_partition *)k_malloc(sizeof(struct vm_mem_partition));
    if (!vpart) {
        return NULL;
    }
    vpart->vm_mm_partition = (struct k_mem_partition *)
            k_malloc(sizeof(struct k_mem_partition));
    if (!vpart->vm_mm_partition) {
        k_free(vpart);
        return NULL;
    }
    vpart->vm_mm_partition->start = ipbase;
    vpart->vm_mm_partition->size = size;
    vpart->vm_mm_partition->attr.attrs = attrs;


    vpart->part_hpa_base = hpbase;
    vpart->part_hpa_size = size;

    sys_dnode_init(&vpart->vpart_node);
    sys_dlist_init(&vpart->blk_list);

    return vpart;
}

/**
 * @brief init vpart from default device tree.
 */
static int create_vm_mem_vpart(struct vm_mem_domain *vmem_domain, uint64_t hpbase,
                    uint64_t ipbase, uint64_t size, uint32_t attrs)
{
    int ret = 0;
    struct vm_mem_partition *vpart;

    vpart = alloc_vm_mem_partition(hpbase, ipbase, size, attrs);
    if (vpart == NULL) {
        return -EMMAO;
    }
    vpart->vmem_domain = vmem_domain;

    ret = add_idle_vpart(vmem_domain, vpart);

    return ret;
}

static int vm_ram_mem_create(struct vm_mem_domain *vmem_domain)
{
    int ret = 0;
    int type = OS_TYPE_MAX;
    uint64_t va_base, pa_base, kpa_base, size;
    struct  _dnode *d_node, *ds_node;
    struct vm *vm = vmem_domain->vm;
    struct vm_mem_partition *vpart;

    type = vm->os->type;
    switch (type) {
    case OS_TYPE_LINUX:
        va_base = LINUX_VMSYS_BASE;
        size = LINUX_VMSYS_SIZE;
#ifdef CONFIG_VM_DYNAMIC_MEMORY
        kpa_base = (uint64_t)k_malloc(size + CONFIG_MMU_PAGE_SIZE);
        if(kpa_base == 0){
            ZVM_LOG_ERR("The heap memory is not enough\n");
            return -EMMAO;
        }
        pa_base = z_mem_phys_addr((void *)ROUND_UP(kpa_base, CONFIG_MMU_PAGE_SIZE));
#else
        ARG_UNUSED(vpart);
        ARG_UNUSED(kpa_base);
        pa_base = LINUX_VM_IMAGE_BASE;
#endif
        break;
    case OS_TYPE_ZEPHYR:
        va_base = ZEPHYR_VMSYS_BASE;
        size = ZEPHYR_VMSYS_SIZE;
#ifdef CONFIG_VM_DYNAMIC_MEMORY
        kpa_base = (uint64_t)k_malloc(size + CONFIG_MMU_PAGE_SIZE);
        if(kpa_base == 0){
            ZVM_LOG_ERR("The heap memory is not enough\n");
            return -EMMAO;
        }
        pa_base = z_mem_phys_addr((void *)ROUND_UP(kpa_base, CONFIG_MMU_PAGE_SIZE));
#else
        ARG_UNUSED(vpart);
        ARG_UNUSED(kpa_base);
        ARG_UNUSED(d_node);
        ARG_UNUSED(ds_node);
        pa_base = ZEPHYR_VM_IMAGE_BASE;
#endif
        break;
    default:
        return -EMMAO;
        break;
    }

    ret =  create_vm_mem_vpart(vmem_domain, pa_base, va_base, size, MT_VM_NORMAL_MEM);

#ifdef CONFIG_VM_DYNAMIC_MEMORY
    SYS_DLIST_FOR_EACH_NODE_SAFE(&vmem_domain->idle_vpart_list,d_node,ds_node){
        vpart = CONTAINER_OF(d_node,struct vm_mem_partition,vpart_node);
        if(vpart->part_hpa_base == pa_base){
            vpart->part_kpa_base = kpa_base;
            break;
        }
    }
#endif
    return ret;
}

/**
 * @brief Create the dtb memory partition.
 */
static int vm_dtb_mem_create(struct vm_mem_domain *vmem_domain)
{
    int ret = 0;
    ARG_UNUSED(ret);
    uint32_t vm_dtb_size = LINUX_DTB_MEM_SIZE;
    uint64_t vm_dtb_base = LINUX_DTB_MEM_BASE;

    /* Attribute 'MT_VM_DEVICE_MEM' was occer a address size trap, replace with normal memory */
    return create_vm_mem_vpart(vmem_domain, vm_dtb_base, vm_dtb_base,
            vm_dtb_size, MT_VM_NORMAL_MEM);
}

static int vm_init_mem_create(struct vm_mem_domain *vmem_domain)
{
    int ret = 0;
    struct vm *vm = vmem_domain->vm;

    ret = vm_ram_mem_create(vmem_domain);
    if (ret) {
        return ret;
    }

#ifdef CONFIG_DTB_FILE_INPUT
    if(vm->os->type == OS_TYPE_LINUX){
        ret = vm_dtb_mem_create(vmem_domain);
    }
#endif /* CONFIG_DTB_FILE_INPUT */

    return ret;
}

/**
 * @brief Alloc memory block for this vpart, direct use current address.
 */
//  static int alloc_vm_mem_block(struct vm_mem_domain *vmem_dm,
//              struct vm_mem_partition *vpart, uint64_t unit_msize)
//  {
//      ARG_UNUSED(vmem_dm);
//      int i, ret = 0;
//      uint64_t vpart_base, blk_count;
//      struct vm_mem_block *block;

//      vpart_base = vpart->vm_mm_partition->start;

//      /* Add flag for blk map, set size as 64k(2M) block */
//      vpart->area_attrs |= BLK_MAP;
//      blk_count = vpart->vm_mm_partition->size / unit_msize;

//      /* allocate physical memory for block */
//      for (i = 0; i < blk_count; i++) {
//          /* allocate block for block struct*/
//          block = (struct vm_mem_block *)k_malloc(sizeof(struct vm_mem_block));
//          if (block == NULL) {
//              return -EMMAO;
//          }
//          memset(block, 0, sizeof(struct vm_mem_block));

//          /* init block pointer for vm */
//          block->virt_base = vpart_base + unit_msize*i;
//          /* get the block number */
//          block->cur_blk_offset = i;
//          /* No physical base */
//          block->phy_pointer = NULL;

//          sys_dlist_append(&vpart->blk_list, &block->vblk_node);
//      }

//      return ret;
//  }


int vm_vdev_mem_create(struct vm_mem_domain *vmem_domain, uint64_t hpbase,
                    uint64_t ipbase, uint64_t size, uint32_t attrs)
{
    return create_vm_mem_vpart(vmem_domain, hpbase, ipbase, size, attrs);
}

// int map_vpart_to_block(struct vm_mem_domain *vmem_domain,
//             struct vm_mem_partition *vpart, uint64_t unit_msize)
// {
//     ARG_UNUSED(unit_msize);
//     int ret = 0;
//     uint64_t vm_mem_size;

//     struct vm_mem_block *blk;
//     struct  _dnode *d_node, *ds_node;
//     struct vm *vm = vmem_domain->vm;

//     switch (vm->os->type) {
//     case OS_TYPE_LINUX:
//         vm_mem_size = LINUX_VM_BLOCK_SIZE;
//         break;
//     case OS_TYPE_ZEPHYR:
//         vm_mem_size = ZEPHYR_VM_BLOCK_SIZE;
//         break;
//     default:
//         vm_mem_size = DEFAULT_VM_BLOCK_SIZE;
//         ZVM_LOG_WARN("Unknow os type!");
//         break;
//     }

// #ifdef CONFIG_VM_DYNAMIC_MEMORY
//     uint64_t base_addr = vpart->area_start;
//     uint64_t size = vpart->area_size;
//     uint64_t virt_offset;

//     SYS_DLIST_FOR_EACH_NODE_SAFE(&vpart->blk_list, d_node, ds_node){
//         blk = CONTAINER_OF(d_node, struct vm_mem_block, vblk_node);

//         /* find the virt address for this block */
//         virt_offset = base_addr + (blk->cur_blk_offset * vm_mem_size);

//         size = vm_mem_size;

//         /* add mapping from virt to block physcal address */
//         ret = arch_mmap_vpart_to_block(blk->phy_base, virt_offset, size, MT_VM_NORMAL_MEM);

//     }
// #else
//     SYS_DLIST_FOR_EACH_NODE_SAFE(&vpart->blk_list, d_node, ds_node){
//         blk = CONTAINER_OF(d_node, struct vm_mem_block, vblk_node);

//         if (blk->cur_blk_offset) {
//             continue;
//         }else{
//             ret = arch_mmap_vpart_to_block(blk->phy_base, vpart->area_start,
//                 vpart->area_size, MT_VM_NORMAL_MEM);
//             if (ret) {
//                 return ret;
//             }
//         }
//         break;
//     }
// #endif /* CONFIG_VM_DYNAMIC_MEMORY */
//     /* get the pgd table */
//     vm->arch->vm_pgd_base = (uint64_t)
//             vm->vmem_domain->vm_mm_domain->arch.ptables.base_xlat_table;

//     return ret;
// }


/**
 * @brief unMap virtual addr 'vpart' to physical addr 'block'.
 */
// int unmap_vpart_to_block(struct vm_mem_domain *vmem_domain,
//             struct vm_mem_partition *vpart)
// {
//     ARG_UNUSED(vmem_domain);
//     int ret = 0;

//     struct vm_mem_block *blk;
//     struct _dnode *d_node, *ds_node;

//      /* @TODO free block struct may be use here */
//     SYS_DLIST_FOR_EACH_NODE_SAFE(&vpart->blk_list, d_node, ds_node){
//         blk = CONTAINER_OF(d_node, struct vm_mem_block, vblk_node);

//         if (blk->cur_blk_offset){
//             continue;
//         } else {
//             ret = arch_unmap_vpart_to_block(vpart->area_start, vpart->area_size);
//             if(ret){
//                 return ret;
//             }
//         }
//         break;

//     }

//     return ret;
// }

static int vm_domain_init(struct k_mem_domain *domain, uint8_t num_parts,
		      struct k_mem_partition *parts[], struct vm *vm)
{
	k_spinlock_key_t key;
	int ret = 0;
    uint32_t vmid = vm->vmid;

	if (domain == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if (!(num_parts == 0U || parts != NULL)) {
		ret = -EINVAL;
		goto out;
	}

	if (!(num_parts <= vm_max_partitions)) {
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&z_vm_domain_lock);
	domain->num_partitions = 0U;
	(void)memset(domain->partitions, 0, sizeof(domain->partitions));
	sys_dlist_init(&domain->mem_domain_q);

    ret = arch_vm_mem_domain_init(domain, vmid);
	k_spin_unlock(&z_vm_domain_lock, key);

out:
	return ret;
}

static bool check_vm_add_partition(struct k_mem_domain *domain,
				struct k_mem_partition *part)
{
	int i;
	uintptr_t pstart, pend, dstart, dend;

	if (part->size == 0U) {
		return false;
	}

	pstart = part->start;
	pend = part->start + part->size;

	if (pend <= pstart) {
		return false;
	}

	/* Check that this partition doesn't overlap any existing ones already
	 * in the domain
	 */
	for (i = 0; i < domain->num_partitions; i++) {
		struct k_mem_partition *dpart = &domain->partitions[i];

		if (dpart->size == 0U) {
			/* Unused slot */
			continue;
		}

		dstart = dpart->start;
		dend = dstart + dpart->size;

		if (pend > dstart && dend > pstart) {
			ZVM_LOG_WARN("partition %p base %lx (size %zu) overlaps existing base %lx (size %zu) \n",
				part, part->start, part->size,
				dpart->start, dpart->size);
			return false;
		}
	}

	return true;
}

static int vm_mem_domain_partition_add(struct vm_mem_domain *vmem_dm,
                             struct vm_mem_partition *vpart)
{
	int p_idx;
	int ret = 0;
    uintptr_t phys_start;
    struct k_mem_domain *domain;
    struct k_mem_partition *part;
    struct vm *vm;
    k_spinlock_key_t key;

    phys_start = vpart->part_hpa_base;
    domain = vmem_dm->vm_mm_domain;
    part = vpart->vm_mm_partition;
    vm = vmem_dm->vm;

	if (!check_vm_add_partition(domain, part)) {
		ret = -EINVAL;
		goto out;
	}

	key = k_spin_lock(&vm_mem_domain_lock);

	for (p_idx = 0; p_idx < vm_max_partitions; p_idx++) {
		/* A zero-sized partition denotes it's a free partition */
		if (domain->partitions[p_idx].size == 0U) {
			break;
		}
	}

	if (p_idx >= vm_max_partitions) {
		ret = -ENOSPC;
		goto unlock_out;
	}
	domain->partitions[p_idx].start = part->start;
	domain->partitions[p_idx].size = part->size;
	domain->partitions[p_idx].attr = part->attr;
	domain->num_partitions++;

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
	ret = arch_vm_mem_domain_partition_add(domain, p_idx, phys_start, vm->vmid);
#endif /* CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API */

unlock_out:
	k_spin_unlock(&vm_mem_domain_lock, key);

out:
	return ret;
}

static int vm_mem_domain_partition_remove(struct vm_mem_domain *vmem_dm)
{
    int p_idx;
    int ret = 0;
    uintptr_t phys_start;
    ARG_UNUSED(phys_start);
    struct k_mem_domain *domain;
    struct vm *vm;
    k_spinlock_key_t key;

    domain = vmem_dm->vm_mm_domain;
    vm = vmem_dm->vm;
    key = k_spin_lock(&vm_mem_domain_lock);

#ifdef CONFIG_ARCH_MEM_DOMAIN_SYNCHRONOUS_API
    for(p_idx = 0;p_idx < vm_max_partitions; p_idx++) {
        if(domain->partitions[p_idx].size != 0U){
            ret = arch_vm_mem_domain_partition_remove(domain,p_idx,vm->vmid);
        }
    }
#endif
    k_free(domain);
    k_spin_unlock(&vm_mem_domain_lock,key);

    return ret;
}

int vm_mem_domain_partitions_add(struct vm_mem_domain *vmem_dm)
{
    int ret = 0;
    k_spinlock_key_t key;
    struct  _dnode *d_node, *ds_node;
    struct vm_mem_partition *vpart;

    key = k_spin_lock(&vmem_dm->spin_mmlock);
    SYS_DLIST_FOR_EACH_NODE_SAFE(&vmem_dm->idle_vpart_list, d_node, ds_node){
        vpart = CONTAINER_OF(d_node, struct vm_mem_partition, vpart_node);
        ZVM_LOG_INFO("\n Ready to add partition to vm memory region. \n ");
        ZVM_LOG_INFO("\n Partition physical addr: %08x, size: %08x. \n ", vpart->part_hpa_base, vpart->part_hpa_size);
        ret = vm_mem_domain_partition_add(vmem_dm, vpart);
        if (ret) {
            k_spin_unlock(&vmem_dm->spin_mmlock, key);
            return ret;
        }

        sys_dlist_remove(&vpart->vpart_node);
        sys_dlist_append(&vmem_dm->mapped_vpart_list, &vpart->vpart_node);
    }

    k_spin_unlock(&vmem_dm->spin_mmlock, key);
    return ret;
}

int vm_mem_apart_remove(struct vm_mem_domain *vmem_dm)
{
    int ret = 0;
    k_spinlock_key_t key;
    struct _dnode *d_node, *ds_node;
    struct vm_mem_partition *vpart;
    struct k_mem_partition  *vmpart;
    struct k_mem_domain  *vm_mem_dm;
    struct vm *vm;

    vm = vmem_dm->vm;
    key = k_spin_lock(&vmem_dm->spin_mmlock);

    vm_mem_dm = vmem_dm->vm_mm_domain;
    ret = vm_mem_domain_partition_remove(vmem_dm);
    SYS_DLIST_FOR_EACH_NODE_SAFE(&vmem_dm->mapped_vpart_list, d_node, ds_node){
        vpart = CONTAINER_OF(d_node, struct vm_mem_partition, vpart_node);
        vmpart = vpart->vm_mm_partition;
    #ifdef CONFIG_VM_DYNAMIC_MEMORY
        if( (vm->vmid < ZVM_ZEPHYR_VM_NUM && vpart->part_hpa_size == ZEPHYR_VMSYS_SIZE) ||
           (vm->vmid >= ZVM_ZEPHYR_VM_NUM && vpart->part_hpa_size == LINUX_VMSYS_SIZE) ){
                k_free((void *)vpart->part_kpa_base);
           }
    #endif
        sys_dlist_remove(&vpart->vpart_node);
        k_free(vmpart);
        k_free(vpart);
    }

    k_spin_unlock(&vmem_dm->spin_mmlock,key);
    return ret;
}


/**
 * @brief add vmem_dm apart to this vm.
 */
int vm_dynmem_apart_add(struct vm_mem_domain *vmem_dm)
{
    int ret = 0;
    uint64_t vm_mem_blk_size;
    k_spinlock_key_t key;
    struct  _dnode *d_node, *ds_node;
    struct vm_mem_partition *vpart;
    struct vm *vm = vmem_dm->vm;

    switch (vm->os->type) {
    case OS_TYPE_LINUX:
        vm_mem_blk_size = LINUX_VM_BLOCK_SIZE;
        break;
    case OS_TYPE_ZEPHYR:
        vm_mem_blk_size = ZEPHYR_VM_BLOCK_SIZE;
        break;
    default:
        vm_mem_blk_size = DEFAULT_VM_BLOCK_SIZE;
        ZVM_LOG_WARN("Unknow os type!\n");
        break;
    }

    key = k_spin_lock(&vmem_dm->spin_mmlock);

    SYS_DLIST_FOR_EACH_NODE_SAFE(&vmem_dm->idle_vpart_list, d_node, ds_node){
        /*TODO: Need a judge for all.*/
        vpart = CONTAINER_OF(d_node, struct vm_mem_partition, vpart_node);

//        ret = alloc_vm_mem_block(vmem_dm, vpart, vm_mem_blk_size);
        if(ret){
            ZVM_LOG_WARN("Init vm memory failed!\n");
            return ret;
        }

        sys_dlist_remove(&vpart->vpart_node);
        sys_dlist_append(&vmem_dm->mapped_vpart_list, &vpart->vpart_node);

    }
    k_spin_unlock(&vmem_dm->spin_mmlock, key);

    return ret;
}

int vm_mem_domain_create(struct vm *vm)
{
    int ret;
    k_spinlock_key_t key;
    struct vm_mem_partition *vpart;
    ARG_UNUSED(vpart);
    struct vm_mem_domain *vmem_dm = vm->vmem_domain;

    /* vm'mm struct init here */
    vmem_dm = (struct vm_mem_domain *)k_malloc(sizeof(struct vm_mem_domain));
    if (!vmem_dm) {
        ZVM_LOG_WARN("Allocate mm memory for vm mm struct failed! \n");
        return -EMMAO;
    }
    vmem_dm->vm_mm_domain = (struct k_mem_domain *)k_malloc(sizeof(struct k_mem_domain));
    if (!vmem_dm->vm_mm_domain) {
        ZVM_LOG_WARN("Allocate mm memory domain failed! \n");
        return -EMMAO;
    }
    vmem_dm->is_init = false;
    ZVM_SPINLOCK_INIT(&vmem_dm->spin_mmlock);
    /* init the list of used and unused vpart */
    sys_dlist_init(&vmem_dm->idle_vpart_list);
    sys_dlist_init(&vmem_dm->mapped_vpart_list);
    ret = vm_domain_init(vmem_dm->vm_mm_domain, 0, NULL, vm);
    if (ret) {
        ZVM_LOG_WARN("Init vm domain failed! \n");
        return -EMMAO;
    }

    vmem_dm->vm = vm;
    vm->vmem_domain = vmem_dm;

    key = k_spin_lock(&vmem_dm->spin_mmlock);
    ret = vm_init_mem_create(vmem_dm);
    if (ret) {
        ZVM_LOG_WARN("Init vm areas failed! \n");
        k_spin_unlock(&vmem_dm->spin_mmlock, key);
        return ret;
    }
    k_spin_unlock(&vmem_dm->spin_mmlock, key);

    return 0;
}
