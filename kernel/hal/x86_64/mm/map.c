#include <hal/mm/map.h>
#include <hal/hardware/reg.h>
#include <mm/dmas.h>
#include <mm/map.h>
#include <mm/dmas.h>
#include <screen/screen.h>
#include <task/api.h>
#include <lib/string.h>

static __always_inline__ u64 _cvtAttr(u64 attr) {
    u64 cvt = 0x1;
    if (attr & mm_Attr_Shared2U)    cvt |= (1ul << 2);
    if (attr & mm_Attr_Writable)    cvt |= (1ul << 1);
    if (attr & mm_Attr_Exist)       cvt |= (1ul << 0);
    return cvt;
}


void hal_mm_dbg(u64 virt) {
    printk(WHITE, BLACK, "mm: map: dbg(): virt: %#018lx\n", virt);
    hal_mm_PageTbl *tbl, *subTbl;

    tbl = virt >= task_krlAddrSt ? mm_dmas_phys2Virt(mm_krlTblPhysAddr) : mm_dmas_phys2Virt(hal_hw_getCR(3));
    printk(WHITE, BLACK, " \tpgd tbl: %#018lx\n", tbl);

    u64 idx = (virt & hal_mm_pgdIdxMask) >> hal_mm_pgdShift;
    tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);
    if (!tbl) {
        printk(WHITE, BLACK, " \tpud: NULL\n");
        return;
    }
    printk(WHITE, BLACK, " \tpud tbl: %#018lx\n", tbl);

    idx = (virt & hal_mm_pudIdxMask) >> hal_mm_pudShift;
    
    tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);
    if (tbl->entries[idx] & 0x80) {
        printk(WHITE, BLACK, " \t1G mapping: %#018lx\n", tbl->entries[idx]);
        return;
    }
    if (!tbl) {
        printk(WHITE, BLACK, " \tpmd tbl: NULL\n");
        return;
    }
    printk(WHITE, BLACK, " \tpmd tbl: %#018lx\n", tbl);
    
    idx = (virt & hal_mm_pmdIdxMask) >> hal_mm_pmdShift;
    if (tbl->entries[idx] & 0x80) {
        printk(WHITE, BLACK, " \t2M mapping: %#018lx\n", tbl->entries[idx]);
        return;
    }
    tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);
    if (!tbl) {
        printk(WHITE, BLACK, " \tpld tbl: NULL\n");
        return;
    }
    printk(WHITE, BLACK, " \tpld tbl: %#018lx\n", tbl);
    
    idx = (virt & hal_mm_pldIdxMask) >> hal_mm_pldShift;
    printk(WHITE, BLACK, " \tphy addr: %#018lx\n", tbl->entries[idx]);
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

int hal_mm_map_syncKrl() {
    memcpy(
        &((hal_mm_PageTbl *)mm_dmas_phys2Virt(mm_krlTblPhysAddr))->entries[256], 
        &((hal_mm_PageTbl *)mm_dmas_phys2Virt(hal_hw_getCR(3)))->entries[256],
        sizeof(u64) * 256);
	return 0;
}

int hal_mm_map_clrTbl(u64 pgd) {
    if (pgd == mm_krlTblPhysAddr) return res_SUCC;
    hal_mm_PageTbl *pgdTbl = mm_dmas_phys2Virt(pgd), *pudTbl, *pmdTbl, *pldTbl;
    for (int i = 0; i < 256; i++) if (pgdTbl->entries[i]) {
        pudTbl = mm_dmas_phys2Virt(pgdTbl->entries[i] & ~0xffful);
        if (mm_dmas_phys2Virt(NULL) == pudTbl || (pmdTbl->entries[i] & 0x80ul)) continue;
        for (int j = 0; j < hal_mm_nrPageTblEntries; j++) if (pudTbl->entries[j]) {
            pmdTbl = mm_dmas_phys2Virt(pudTbl->entries[j] & ~0xffful);
            if (mm_dmas_phys2Virt(NULL) == pmdTbl || (pmdTbl->entries[j] & 0x80ul)) continue;
            for (int k = 0; k < hal_mm_nrPageTblEntries; k++) if (pmdTbl->entries[k]) {
                pldTbl = mm_dmas_phys2Virt(pmdTbl->entries[j] & ~0xffful);
                if (mm_dmas_phys2Virt(NULL) == pldTbl) continue;
                if (mm_map_freeTbl(pldTbl) == res_FAIL) return res_FAIL;
            }
            if (mm_map_freeTbl(pmdTbl) == res_FAIL) return res_FAIL;
        }
        if (mm_map_freeTbl(pudTbl) == res_FAIL) return res_FAIL;
    }
    return mm_map_freeTbl(pgdTbl);
}