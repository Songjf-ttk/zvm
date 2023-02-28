/*
 * Copyright (c) 2022 xcl (xiongcl@hnu.edu.cn)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_ARM_MM_H_
#define ZEPHYR_INCLUDE_ZVM_ARM_MM_H_

#include <zephyr.h>
#include <stdint.h>
#include <sys/dlist.h>
#include <sys/util.h>
#include <_zvm/vm.h>
#include <_zvm/vm_mm.h>

/**
 * stage-2 Memory types supported through MAIR.
 */
#define MT_S2_TYPE_MASK			0xFU
#define MT_S2_TYPE(attr)		(attr & MT_TYPE_MASK)
#define MT_S2_DEVICE_nGnRnE		0U
#define MT_S2_DEVICE_nGnRE		1U
#define MT_S2_DEVICE_GRE		2U
#define MT_S2_NORMAL_NC			3U
#define MT_S2_NORMAL			4U
#define MT_S2_NORMAL_WT			5U

/* Reuse host's mair for configure */
#define MEMORY_S2_ATTRIBUTES	((0x00 << (MT_S2_DEVICE_nGnRnE * 8)) |	\
				(0x04 << (MT_S2_DEVICE_nGnRE * 8))   |	\
				(0x0c << (MT_S2_DEVICE_GRE * 8))     |	\
				(0x44 << (MT_S2_NORMAL_NC * 8))      |	\
				(0xffUL << (MT_S2_NORMAL * 8))	  |	\
				(0xbbUL << (MT_S2_NORMAL_WT * 8)))

/* More flags from user's perpective are supported using remaining bits
 * of "attrs" field, i.e. attrs[31:4], underlying code will take care
 * of setting PTE fields correctly.
 */
#define  MT_S2_PERM_R_SHIFT			4U
#define  MT_S2_PERM_W_SHIFT			5U
#define  MT_S2_EXECUTE_SHIFT		6U
#define	 MT_S2_NOACCESS_SHIFT		7U


#define MT_S2_NR				(0U << MT_S2_PERM_R_SHIFT)
#define MT_S2_R					(1U << MT_S2_PERM_R_SHIFT)

#define MT_S2_NW				(0U << MT_S2_PERM_W_SHIFT)
#define MT_S2_W					(1U << MT_S2_PERM_W_SHIFT)

#define MT_S2_EXECUTE_NEVER		(0U << MT_S2_EXECUTE_SHIFT)
#define MT_S2_EXECUTE			(1U << MT_S2_EXECUTE_SHIFT)

#define MT_S2_ACCESS_ON			(0U << MT_S2_NOACCESS_SHIFT)
#define MT_S2_ACCESS_OFF		(1U << MT_S2_NOACCESS_SHIFT)


#define MT_S2_P_RW_U_RW_NXN		(MT_S2_R | MT_S2_W | MT_S2_EXECUTE)
#define MT_S2_P_RW_U_RW_XN		(MT_S2_R | MT_S2_W | MT_S2_EXECUTE_NEVER)


#define MT_VM_NORMAL_MEM		(MT_S2_P_RW_U_RW_NXN | MT_S2_NORMAL)
#define MT_VM_DEVICE_MEM		(MT_S2_P_RW_U_RW_XN | MT_S2_DEVICE_GRE)


/*
 * Block and Page descriptor attributes fields for stage-2
 */
#define S2_PTE_BLOCK_DESC_MEMTYPE(x)	(x << 2)
#define S2_PTE_BLOCK_DESC_AP_NO_RW		(0ULL << 6)
#define S2_PTE_BLOCK_DESC_AP_RO			(1ULL << 6)
#define S2_PTE_BLOCK_DESC_AP_WO			(2ULL << 6)
#define S2_PTE_BLOCK_DESC_AP_RW			(3ULL << 6)
#define S2_PTE_BLOCK_DESC_NON_SHARE		(0ULL << 8)
#define S2_PTE_BLOCK_DESC_OUTER_SHARE	(2ULL << 8)
#define S2_PTE_BLOCK_DESC_INNER_SHARE	(3ULL << 8)
#define S2_PTE_BLOCK_DESC_AF			(1ULL << 10)
#define S2_PTE_BLOCK_DESC_XS			(0ULL << 11)
#define S2_PTE_BLOCK_DESC_NO_XN			(0ULL << 53)
#define S2_PTE_BLOCK_DESC_P_XN			(1ULL << 53)
#define S2_PTE_BLOCK_DESC_PU_XN			(2ULL << 53)
#define S2_PTE_BLOCK_DESC_U_XN			(3ULL << 53)

/**
 * @brief Mapping vpart to physical block address.
 */
int arch_mmap_vpart_to_block(uintptr_t phys, uintptr_t virt, size_t size, uint32_t attrs);
int arch_unmap_vpart_to_block(uintptr_t virt, size_t size);

/**
 * @brief Add dev io memory map for the VM.
 */
int arch_vm_dev_domain_map(uint64_t pbase, uint64_t vbase, uint64_t size, char *name, struct vm *vm);
int arch_vm_dev_domain_unmap(uint64_t pbase, uint64_t vbase, uint64_t size, char *name, struct vm *vm);

/**
 * @brief map vma to physical block address:
 * this function aim to translate virt address to phys address by setting the
 * hyp page table.
 */
int arch_mmap_vma_to_block(uintptr_t phys, uintptr_t virt, size_t size, uint32_t attrs);
int arch_unmap_vma_to_block(uintptr_t virt, size_t size);

/**
 * @brief Add a partition to the vm virtual memory domain.
 */
int arch_vm_mem_domain_partition_add(struct k_mem_domain *domain,
				  uint32_t partition_id, uintptr_t phys_start, uint32_t vmid);

/**
 * @brief Architecture-specific hook for vm domain initialization.
 */
int arch_mem_domain_init(struct k_mem_domain *domain);

#endif /* ZEPHYR_INCLUDE_ZVM_ARM_MM_H_ */
