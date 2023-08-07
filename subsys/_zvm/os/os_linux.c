/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <_zvm/zvm.h>
#include <_zvm/vm_manager.h>
#include <_zvm/os/os_linux.h>


/**
 * @brief load linux image from other memory address to allocated address
 */
int load_linux_image(struct vm_mem_domain *vmem_domain)
{
    char *dbuf, *sbuf;
    int ret = 0;
    uint64_t zbase_addr, zsize;
    struct _dnode *d_node, *ds_node;
    struct vm_mem_partition *vpart;
    struct vm_mem_block *blk;
    struct vm *this_vm = vmem_domain->vm;

#ifndef  CONFIG_VM_DYNAMIC_MEMORY
    ARG_UNUSED(this_vm);
    return ret;
#endif /* CONFIG_VM_DYNAMIC_MEMORY */

    /*Find the zephyr image base_addr and it size here */
    // zbase_addr = LINUX_VM_MEM_BASE;
    // zsize = LINUX_VM_MEM_SIZE;

    // /* Find the related vpart for this vm */
    // SYS_DLIST_FOR_EACH_NODE_SAFE(&vmem_domain->mapped_vpart_list, d_node, ds_node){
    //     vpart = CONTAINER_OF(d_node, struct vm_mem_partition, vpart_node);
    //     /* find the related vtme */
    //     if (vpart->area_size == zsize) {
    //         // relocate memory for each block
    //             SYS_DLIST_FOR_EACH_NODE_SAFE(&vpart->blk_list, d_node, ds_node){
    //             blk = CONTAINER_OF(d_node, struct vm_mem_block, vblk_node);

    //             sbuf = (char *)(zbase_addr + blk->cur_blk_offset * LINUX_VM_BLOCK_SIZE);
    //             dbuf = (char *)blk->phy_base;
    //             memcpy(dbuf, sbuf, LINUX_VM_BLOCK_SIZE);

    //             zsize = zsize - LINUX_VM_BLOCK_SIZE;
    //         }
    //         break;
    //     } else {
    //         continue;
    //     }

    // }
    // if (zsize) {
    //     ret = -EIO;
    // }

    return ret;

}
