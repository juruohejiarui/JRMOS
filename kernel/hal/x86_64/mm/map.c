#include <hal/mm/map.h>
#include <hal/hardware/reg.h>
#include <mm/dmas.h>
#include <mm/map.h>
#include <mm/dmas.h>
#include <screen/screen.h>
#include <task/api.h>

static __always_inline__ u64 _cvtAttr(u64 attr) {
    u64 cvt = 0x1;
    if (attr & mm_Attr_Shared2U)    cvt |= (1ul << 2);
    if (attr & mm_Attr_Exist)       cvt |= (1ul << 0);
    return cvt;
}
 
int hal_mm_map(u64 virt, u64 phys, u64 attr) {
    attr = _cvtAttr(attr);
    hal_mm_PageTbl *tbl, *subTbl;

    tbl = virt >= task_krlAddrSt ? mm_dmas_phys2Virt(mm_krlTblPhysAddr) : mm_dmas_phys2Virt(hal_hw_getCR(3));

    u64 idx = (virt & hal_mm_pgdIdxMask) >> hal_mm_pgdShift;
    if (!tbl->entries[idx]) {
        if ((subTbl = mm_map_allocTbl()) == NULL) return res_FAIL;
        tbl->entries[idx] = mm_dmas_virt2Phys(subTbl) | 0x7;
        tbl = subTbl;
    } else tbl = (void *)(tbl->entries[idx] & ~0xffful);
    
    idx = (virt & hal_mm_pudIdxMask) >> hal_mm_pudShift;
    if (!tbl->entries[idx]) {
        if ((subTbl = mm_map_allocTbl()) == NULL) return res_FAIL;
        tbl->entries[idx] = mm_dmas_virt2Phys(subTbl) | 0x7;
        tbl = subTbl;
    } else tbl = (void *)(tbl->entries[idx] & ~0xffful);

    idx = (virt & hal_mm_pmdIdxMask) >> hal_mm_pmdShift;
    if (!tbl->entries[idx]) {
        if ((subTbl = mm_map_allocTbl()) == NULL) return res_FAIL;
        tbl->entries[idx] = mm_dmas_virt2Phys(subTbl) | 0x7;
        tbl = subTbl;
    } else tbl = (void *)(tbl->entries[idx] & ~0xffful);

    idx = (virt & hal_mm_pldIdxMask) >> hal_mm_pldShift;
    tbl->entries[idx] = phys | attr;

    hal_mm_flushTlb();

    return res_SUCC;
}

int hal_mm_map1G(u64 virt, u64 phys, u64 attr) {
    attr = _cvtAttr(attr);
    hal_mm_PageTbl *tbl, *subTbl;

    tbl = virt >= task_krlAddrSt ? mm_dmas_phys2Virt(mm_krlTblPhysAddr) : mm_dmas_phys2Virt(hal_hw_getCR(3));

    u64 idx = (virt & hal_mm_pgdIdxMask) >> hal_mm_pgdShift;
    if (!tbl->entries[idx]) {
        if ((subTbl = mm_map_allocTbl()) == NULL) return res_FAIL;
        tbl->entries[idx] = mm_dmas_virt2Phys(subTbl) | 0x7;
        tbl = subTbl;
    } else tbl = (void *)(tbl->entries[idx] & ~0xffful);

    idx = (virt & hal_mm_pudIdxMask) >> hal_mm_pudShift;
    if (tbl->entries[idx]) {
        printk(RED, BLACK, "mm: map: map1G(): failed to map %#018lx to %#018lx: exist pud entry: %#018lx\n", virt, phys, tbl->entries[idx]);
        return res_FAIL;
    }
    tbl->entries[idx] = phys | attr | 0x80;

    hal_mm_flushTlb();

    return res_SUCC;
}

int hal_mm_unmap(u64 virt) {
    hal_mm_PageTbl *tbl, *subTbl;

    tbl = virt >= task_krlAddrSt ? mm_dmas_phys2Virt(mm_krlTblPhysAddr) : mm_dmas_phys2Virt(hal_hw_getCR(3));

    u64 idx = (virt & hal_mm_pgdIdxMask) >> hal_mm_pgdShift;
    tbl = (hal_mm_PageTbl *)(tbl->entries[idx] & ~0xffful);
    if (tbl == NULL) return res_FAIL;

    idx = (virt & hal_mm_pudIdxMask) >> hal_mm_pudShift;
    if (tbl->entries[idx] & 0x80) {
        printk(RED, BLACK, "mm: map: unmap(): failed to unmap %#018lx. part of 1G mapping.\n",
            virt);
        return res_FAIL;
    }
    tbl = (hal_mm_PageTbl *)(tbl->entries[idx] & ~0xffful);
    if (tbl == NULL) return res_FAIL;

    idx = (virt & hal_mm_pmdIdxMask) >> hal_mm_pmdShift;
    if (tbl->entries[idx] & 0x80) {
        printk(RED, BLACK, "mm: map: unmap(): failed to unmap %#018lx. part of 2M mapping.\n",
            virt);
        return res_FAIL;
    }
    tbl = (hal_mm_PageTbl *)(tbl->entries[idx] & ~0xffful);

    idx = (virt & hal_mm_pldIdxMask) >> hal_mm_pldShift;
    tbl->entries[idx] = 0;

    hal_mm_flushTlb();

    return res_SUCC;
}

u64 hal_mm_getMap(u64 virt) {
    hal_mm_PageTbl *tbl, *subTbl;

    tbl = virt >= task_krlAddrSt ? mm_dmas_phys2Virt(mm_krlTblPhysAddr) : mm_dmas_phys2Virt(hal_hw_getCR(3));

    u64 idx = (virt & hal_mm_pgdIdxMask) >> hal_mm_pgdShift;
    tbl = (hal_mm_PageTbl *)(tbl->entries[idx] & ~0xffful);
    if (tbl == NULL) return res_FAIL;

    idx = (virt & hal_mm_pudIdxMask) >> hal_mm_pudShift;
    if (~tbl->entries[idx] & 0x1) return 0;
    if (tbl->entries[idx] & 0x80)
        return (tbl->entries[idx] & ~0xffful) | (virt & ((1ul << hal_mm_pudShift) - 1));
    tbl = (hal_mm_PageTbl *)(tbl->entries[idx] & ~0xffful);

    idx = (virt & hal_mm_pudIdxMask) >> hal_mm_pudShift;
    if (~tbl->entries[idx] & 0x1) return 0;
    if (tbl->entries[idx] & 0x80)
        return (tbl->entries[idx] & ~0xffful) | (virt & ((1ul << hal_mm_pudShift) - 1));
    tbl = (hal_mm_PageTbl *)(tbl->entries[idx] & ~0xffful);

    idx = (virt & hal_mm_pmdIdxMask) >> hal_mm_pmdShift;
    if (~tbl->entries[idx] & 0x1) return 0;
    if (tbl->entries[idx] & 0x80)
        return (tbl->entries[idx] & ~0xffful) | (virt & ((1ul << hal_mm_pmdShift) - 1));
    tbl = (hal_mm_PageTbl *)(tbl->entries[idx] & ~0xffful);

    idx = (virt & hal_mm_pldIdxMask) >> hal_mm_pldShift;
    if (~tbl->entries[idx] & 0x1) return 0;
    return (tbl->entries[idx] & ~0xffful) | (virt & ((1ul << hal_mm_pldShift) - 1));
}

int hal_mm_map_syncKrl()
{
	return 0;
}