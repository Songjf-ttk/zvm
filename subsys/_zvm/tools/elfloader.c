/*
 * Copyright (c) 2022 xcl (xiongcl@hnu.edu.cn)
 * 
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <_zvm/tools/elfloader.h>
#include <_zvm/tools/elf.h>
#include <_zvm/zvm.h>

LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define EL_PHOFF(ctx, num) (((ctx)->ehdr.e_phoff + (num) * (ctx)->ehdr.e_phentsize))

extern el_status el_applyrel(el_ctx *ctx, Elf_Rel *rel);
extern el_status el_applyrela(el_ctx *ctx, Elf_RelA *rela);

/* virtual vm image base addr */
void *buf;

/**
 * @brief Read nb bytes from *src to *dest
 * @return true : success
 * @return false : failed
 */
static bool file_pread(el_ctx *ctx, void *dest, void *src, size_t nb)
{
    ARG_UNUSED(ctx);

    if (memcpy(dest, src, nb) == NULL){
        return false;
    }

    return true;
}

static void *alloccb(el_ctx *ctx, Elf_Addr phys, Elf_Addr virt, Elf_Addr size)
{
    ARG_UNUSED(ctx);
    ARG_UNUSED(phys);
    ARG_UNUSED(size);

    return (void*) virt;
}

static el_status el_pread(el_ctx *ctx, void *dest, void *src, size_t nb)
{
    return ctx->pread(ctx, dest, src, nb) ? EL_OK : EL_EIO;
}


/* Find the next phdr of type \p type, starting at \p *i.
 * On success, returns EL_OK with *i set to the phdr number, and the phdr loadelf_findphdred
 * in *phdr.
 *
 * If the end of the phdrs table was reached, *i is set to -1 and the contents
 * of *phdr are undefined
 */
static el_status elf_findphdr(el_ctx *ctx, Elf_Phdr *phdr, uint32_t type, unsigned *i, void *src)
{
    el_status rv = EL_OK;
    for (; *i < ctx->ehdr.e_phnum; (*i)++) {
        if ((rv = el_pread(ctx, phdr, src+EL_PHOFF(ctx, *i),  sizeof *phdr ))){
            return rv;
        }

        if (phdr->p_type == type) {
            return rv;
        }
    }

    *i = -1;
    return rv;
}

static el_status elf_init(el_ctx *ctx, void *src)
{
    unsigned i = 0;
    Elf_Phdr ph;
    el_status rv = EL_OK;

    if ((rv = el_pread(ctx, &ctx->ehdr, src , sizeof ctx->ehdr))){
        return rv;
    }

    /* validate header */
    if (!IS_ELF(ctx->ehdr)){
        return EL_NOTELF;
    }

    if (ctx->ehdr.e_ident[EI_CLASS] != ELFCLASS){
        return EL_WRONGBITS;
    }

    if (ctx->ehdr.e_ident[EI_DATA] != ELFDATATHIS){
        return EL_WRONGENDIAN;
    }

    if (ctx->ehdr.e_ident[EI_VERSION] != EV_CURRENT){
        return EL_NOTELF;
    }

    if (ctx->ehdr.e_type != ET_EXEC && ctx->ehdr.e_type != ET_DYN){
        return EL_NOTEXEC;
    }

    if (ctx->ehdr.e_machine != EM_THIS){
        return EL_WRONGARCH;
    }

    if (ctx->ehdr.e_version != EV_CURRENT){
        return EL_NOTELF;
    }


    /* iterate through, calculate extents */
    ctx->base_load_paddr = ctx->base_load_vaddr = 0;
    ctx->align = 1;
    ctx->memsz = 0;

    for(;;) {
        if ((rv = elf_findphdr(ctx, &ph, PT_LOAD, &i, src))){
            return rv;
        }

        if (i == (unsigned) -1){
            break;
        }
        /**
         * When looking it in virtual space, the end of address is ph.p_vaddr + ph.p_memsz,
         * but in the zephyr system, such big a block can not allocated by system. So we must calculate
         * the minus of the end of address and the begin of address!
         */
        Elf_Addr phend = ph.p_vaddr + ph.p_memsz;
        if (phend > ctx->memsz){
            ctx->memsz = phend;
        }

        if (ph.p_align > ctx->align){
            ctx->align = ph.p_align;
        }

        i++;
    }

    if (ctx->ehdr.e_type == ET_DYN) {
        i = 0;

        if ((rv = elf_findphdr(ctx, &ph, PT_DYNAMIC, &i, src))){
            return rv;
        }

        if (i == (unsigned) -1){
            return EL_NODYN;
        }

        ctx->dynoff  = ph.p_offset;
        ctx->dynsize = ph.p_filesz;
    } else {
        ctx->dynoff  = 0;
        ctx->dynsize = 0;
    }

    return rv;
}


static el_status elf_load(el_ctx *ctx, el_alloc_cb alloc, void *src)
{
    el_status rv = EL_OK;

    /* address deltas */
    Elf_Addr pdelta = ctx->base_load_paddr;
    Elf_Addr vdelta = ctx->base_load_vaddr;

    /* iterate paddrs */
    Elf_Phdr ph;
    unsigned i = 0;
    for(;;) {
        if ((rv = elf_findphdr(ctx, &ph, PT_LOAD, &i, src))){
            return rv;
        }

        if (i == (unsigned) -1){
            break;
        }
        /* We just need to load src+offset to dest that has been
           allocated before*/
        /**
        Elf_Addr pload = ph.p_paddr + pdelta;
        Elf_Addr vload = ph.p_vaddr + vdelta;
        */
        Elf_Addr pload = ph.p_paddr + pdelta - (uint64_t)src;
        Elf_Addr vload = ph.p_vaddr + vdelta - (uint64_t)src;

        /* allocate mem */
        char *dest = alloc(ctx, pload, vload, ph.p_memsz);
        if (!dest){
            return EL_ENOMEM;
        }

        ZVM_LOG_INFO("\nLoading seg fileoff %x, vaddr %x to %p\n",
            ph.p_offset, ph.p_vaddr, dest);

        /* read loaded portion */
        if ((rv = el_pread(ctx, dest, src+ph.p_offset, ph.p_filesz))){
            return rv;
        }

        /* zero mem-only portion */
        memset(dest + ph.p_filesz, 0, ph.p_memsz - ph.p_filesz);

        i++;
    }
    return rv;
}

/**
 * find a dynamic table entry
 * returns the entry on success, dyn->d_tag = DT_NULL on failure
 */
static el_status elf_finddyn(el_ctx *ctx, Elf_Dyn *dyn, uint32_t tag, void *src)
{
    el_status rv = EL_OK;
    size_t ndyn =  ctx->dynsize / sizeof(Elf_Dyn);

    for(unsigned i = 0; i < ndyn; i++) {
        if ((rv = el_pread(ctx, dyn, src+(ctx->dynoff + i * sizeof *dyn), sizeof *dyn ))){
            return rv;
        }

        if (dyn->d_tag == tag){
            return EL_OK;
        }
    }

    dyn->d_tag = DT_NULL;
    return EL_OK;
}

/* find all information regarding relocations of a specific type.
 * pass DT_REL or DT_RELA for type
 * sets ri->entrysize = 0 if not found
 */
static el_status elf_findrelocs(el_ctx *ctx, el_relocinfo *ri, uint32_t type, void *src)
{
    el_status rv = EL_OK;

    Elf_Dyn rel, relsz, relent;

    if ((rv = elf_finddyn(ctx, &rel, type, src))){
        return rv;
    }

    if ((rv = elf_finddyn(ctx, &relsz, type + 1, src))){
        return rv;
    }

    if ((rv = elf_finddyn(ctx, &relent, type + 2, src))){
        return rv;
    }

    if (rel.d_tag == DT_NULL
            || relsz.d_tag == DT_NULL
            || relent.d_tag == DT_NULL) {
        ri->entrysize = 0;
        ri->tablesize = 0;
        ri->tableoff  = 0;
    } else {
        ri->tableoff  = rel.d_un.d_ptr;
        ri->tablesize = relsz.d_un.d_val;
        ri->entrysize = relent.d_un.d_val;
    }

    return rv;
}

/* Relocate the loaded executable */
static el_status elf_relocate(el_ctx *ctx, void *src)
{
    el_status rv = EL_OK;
    el_relocinfo ri;

    // not dynamic
    if (ctx->ehdr.e_type != ET_DYN){
        return EL_OK;
    }

    char *base = (char *) ctx->base_load_paddr;

#ifdef EL_ARCH_USES_REL
    if ((rv = elf_findrelocs(ctx, &ri, DT_REL, src))){
        return rv;
    }

    if (ri.entrysize != sizeof(Elf_Rel) && ri.tablesize) {
        ZVM_LOG_WARN("Relocation size %u doesn't match expected %u\n",
        ri.entrysize, sizeof(Elf_Rel));
        return EL_BADREL;
    }

    size_t relcnt = ri.tablesize / sizeof(Elf_Rel);
    Elf_Rel *reltab = base + ri.tableoff;
    for (size_t i = 0; i < relcnt; i++) {
        if ((rv = el_applyrel(ctx, &reltab[i]))){
            return rv;
        }
    }
#else
    ZVM_LOG_WARN("Architecture doesn't use REL\n");
#endif /* EL_ARCH_USES_REL */

#ifdef EL_ARCH_USES_RELA
    if ((rv = elf_findrelocs(ctx, &ri, DT_RELA, src))) {
        return rv;
    }

    if (ri.entrysize != sizeof(Elf_RelA) && ri.tablesize) {
        ZVM_LOG_WARN("Relocation size %u doesn't match expected %u\n",
            ri.entrysize, sizeof(Elf_RelA));
        return EL_BADREL;
    }

    size_t relacnt = ri.tablesize / sizeof(Elf_RelA);
    Elf_RelA *relatab = (Elf_RelA *)(base + ri.tableoff);
    for (size_t i = 0; i < relacnt; i++) {
        if ((rv = el_applyrela(ctx, &relatab[i]))) {
            return rv;
        }
    }
#else
    ZVM_LOG_WARN("Architecture doesn't use RELA\n");
#endif /* EL_ARCH_USES_RELA */

#if !defined(EL_ARCH_USES_REL) && !defined(EL_ARCH_USES_RELA)
    #error No relocation type defined!
#endif /* EL_ARCH_USES_REL && EL_ARCH_USES_RELA*/

    return rv;
}


/**
 * @brief load vm's elf file for debug it's elf file
 */
int elf_loader(void *src_addr, void *dest_addr, struct z_vm_info *vm_info)
{
    ARG_UNUSED(dest_addr);

    int ret;
    uintptr_t epaddr;
    el_ctx ctx;

    ctx.pread = file_pread;

    /* Init elf struct */
    ret = elf_init(&ctx, src_addr);
    if (ret) {
        ZVM_LOG_WARN("Elf_loader struct init error, status: %d", ret);
        return -ret;
    }

    buf = k_aligned_alloc(ctx.align, ctx.memsz - (uint64_t)src_addr);
    if (!buf) {
        ZVM_LOG_ERR("Memalign error!");
        return -1;
    }

    /* set base load addr for vm elf file */
    ctx.base_load_vaddr = ctx.base_load_paddr = (uintptr_t) buf;

    ret = elf_load(&ctx, alloccb, src_addr);
    if (ret) {
        ZVM_LOG_WARN("Elf_loader addr load error, status: %d", ret);
        return -ret;
    }

    ret = elf_relocate(&ctx, src_addr);
    if (ret) {
        ZVM_LOG_WARN("Elf_loader addr relocatead error, status: %d", ret);
        return -ret;
    }

    /* epaddr = ctx.ehdr.e_entry + (uintptr_t) buf; */
    /* entry point */
    epaddr = ctx.ehdr.e_entry + (uintptr_t) buf - (uintptr_t)src_addr;

    ZVM_LOG_INFO("\n VM's binary entrypoint is %x, the epaddr: %x \n",
            (uintptr_t) ctx.ehdr.e_entry, epaddr);

    vm_info->entry_point = epaddr;

    /* k_free(buf); */

    /* copy the block to aimed address */
    if (memcpy((void*)src_addr, buf, ctx.memsz - (uint64_t)src_addr)== NULL) {
        return -1;
    }

    vm_info->entry_point = epaddr - (uintptr_t)buf + (uintptr_t)src_addr;

    ZVM_LOG_INFO("\n VM's finial entrypoint is %x \n", (uintptr_t)vm_info->entry_point);

    return 0;
}
