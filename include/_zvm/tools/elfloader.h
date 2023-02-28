/*
 * Copyright (c) 2022 xcl (xiongcl@hnu.edu.cn)
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ELFLOADER_H_
#define __ELFLOADER_H_

#include <kernel.h>
#include <stdbool.h>
#include <stddef.h>
#include "elfarch.h"
#include "elf.h"

#include <_zvm/zvm.h>
#include <_zvm/vm.h>

typedef enum {
    EL_OK         = 0,

    EL_EIO,
    EL_ENOMEM,
    EL_NOTELF,
    EL_WRONGBITS,
    EL_WRONGENDIAN,
    EL_WRONGARCH,
    EL_WRONGOS,
    EL_NOTEXEC,
    EL_NODYN,
    EL_BADREL,

} el_status;

typedef struct el_ctx {
    bool (*pread)(struct el_ctx *ctx, void *dest, void *src,  size_t nb);

    /* base_load_* -> address we are actually going to load at
     */
    Elf_Addr
        base_load_paddr,
        base_load_vaddr;

    /* size in memory of binary */
    Elf_Addr memsz;

    /* required alignment */
    Elf_Addr align;

    /* ELF header */
    Elf_Ehdr  ehdr;

    /* Offset of dynamic table (0 if not ET_DYN) */
    Elf_Off  dynoff;
    /* Size of dynamic table (0 if not ET_DYN) */
    Elf_Addr dynsize;
} el_ctx;


typedef void* (*el_alloc_cb)(el_ctx *ctx,Elf_Addr phys,
            Elf_Addr virt, Elf_Addr size);


typedef struct {
    Elf_Off  tableoff;
    Elf_Addr tablesize;
    Elf_Addr entrysize;
} el_relocinfo;


/**
 * @brief load vm's elf file for debug it.
 *
 * @param src_addr : vm' origin image address.
 * @param dest_addr : The vm's memory map addrss.
 * @param vcpu : vcpu strcut.
 * @return int : return code.
 */
int elf_loader(void *src_addr, void *dest_addr,  struct z_vm_info *vm_info);

#endif  /* __ELFLOADER_H_ */
