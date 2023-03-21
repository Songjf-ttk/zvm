/*
 * Copyright 2021-2022 HNU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <os/os.h>
#include <sys/mem_manage.h>
#include <_zvm/zvm.h>
#include <_zvm/arm/mm.h>
#include <_zvm/os/os_zephyr.h>
#include <_zvm/vm_mm.h>


int load_zephyr_image(struct vm_mem_domain *vmem_domain)
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
    zbase_addr = ZEPHYR_VM_MEM_BASE;
    zsize = ZEPHYR_VM_MEM_SIZE;

    /* Find the related vpart for this vm */
    SYS_DLIST_FOR_EACH_NODE_SAFE(&vmem_domain->mapped_vpart_list, d_node, ds_node){
        vpart = CONTAINER_OF(d_node, struct vm_mem_partition, vpart_node);
        /* find the related vtme */
        if (vpart->area_size == zsize) {
                SYS_DLIST_FOR_EACH_NODE_SAFE(&vpart->blk_list, d_node, ds_node){
                blk = CONTAINER_OF(d_node, struct vm_mem_block, vblk_node);

                sbuf = (char *)(zbase_addr + blk->cur_blk_offset * ZEPHYR_VM_BLOCK_SIZE);
                dbuf = (char *)blk->phy_base;
                memcpy(dbuf, sbuf, ZEPHYR_VM_BLOCK_SIZE);

                zsize = zsize - ZEPHYR_VM_BLOCK_SIZE;
            }
            break;
        }else{
            continue;
        }

    }
    if (zsize) {
        ret = -EIO;
    }

    return ret;

}
