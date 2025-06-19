#include <hal/mm/map.h>
#include <hal/hardware/reg.h>
#include <mm/dmas.h>
#include <mm/map.h>
#include <mm/dmas.h>
#include <screen/screen.h>
#include <task/api.h>
#include <lib/string.h>

__always_inline__ u64 _cvtAttr(u64 attr) {
    u64 cvt = 0x1;
    if (attr & mm_Attr_Shared2U)    cvt |= (1ull<< 2);
    if (attr & mm_Attr_Writable)    cvt |= (1ull<< 1);
    if (attr & mm_Attr_Exist)       cvt |= (1ull<< 0);
    return cvt;
}


void hal_mm_map_dbgMap(u64 virt) {
    printk(WHITE, BLACK, "mm: map: dbg(): virt: %p\n", virt);
    hal_mm_PageTbl *tbl, *subTbl;

    tbl = virt >= task_krlAddrSt ? mm_dmas_phys2Virt(mm_krlTblPhysAddr) : mm_dmas_phys2Virt(hal_hw_getCR(3));
    printk(WHITE, BLACK, " \tpgd tbl: %p\n", tbl);

    u64 idx = (virt & hal_mm_pgdIdxMask) >> hal_mm_pgdShift;
    tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);
    if (!(tbl->entries[idx] & ~0xffful)) {
        printk(WHITE, BLACK, " \t[%3x] pud: NULL\n", idx);
        return;
    }
    printk(WHITE, BLACK, " \t[%3x] pud tbl: %p\n", idx, tbl);

    idx = (virt & hal_mm_pudIdxMask) >> hal_mm_pudShift;
    
    tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);
    if (tbl->entries[idx] & 0x80) {
        printk(WHITE, BLACK, " \t[%3x] 1G mapping: %p\n", idx, tbl->entries[idx]);
        return;
    }
    if (!(tbl->entries[idx] & ~0xffful)) {
        printk(WHITE, BLACK, " \t[%3x] pmd tbl: NULL\n", idx);
        return;
    }
    printk(WHITE, BLACK, " \t[%3x] pmd tbl: %p\n", idx, tbl);
    
    idx = (virt & hal_mm_pmdIdxMask) >> hal_mm_pmdShift;
    if (!(tbl->entries[idx] & ~0xffful)) {
        printk(WHITE, BLACK, " \t[%3x] 2M mapping: %p\n", idx, tbl->entries[idx]);
        return;
    }
    tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);
    if (!(tbl->entries[idx] & ~0xffful)) {
        printk(WHITE, BLACK, " \t[%3x] pld tbl: NULL\n", idx);
        return;
    }
    printk(WHITE, BLACK, " \t[%3x] pld tbl: %p\n", idx, tbl);
    
    idx = (virt & hal_mm_pldIdxMask) >> hal_mm_pldShift;
    printk(WHITE, BLACK, " \tphy addr: %p\n", tbl->entries[idx]);
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
    } else tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);
    
    idx = (virt & hal_mm_pudIdxMask) >> hal_mm_pudShift;
    if (!tbl->entries[idx]) {
        if ((subTbl = mm_map_allocTbl()) == NULL) return res_FAIL;
        tbl->entries[idx] = mm_dmas_virt2Phys(subTbl) | 0x7;
        tbl = subTbl;
    } else {
        if (tbl->entries[idx] & 0x80) {
            printk(RED, BLACK, "mm: map: map(): failed to map %p to %p: exist 1G map: %p\n", virt, phys, tbl->entries[idx]);
            return res_FAIL;
        }
        tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);
    }

    idx = (virt & hal_mm_pmdIdxMask) >> hal_mm_pmdShift;
    if (!tbl->entries[idx]) {
        if ((subTbl = mm_map_allocTbl()) == NULL) return res_FAIL;
        tbl->entries[idx] = mm_dmas_virt2Phys(subTbl) | 0x7;
        tbl = subTbl;
    } else {
        if (tbl->entries[idx] & 0x80) {
            printk(RED, BLACK, "mm: map: map(): failed to map %p to %p: exist 2M map: %p\n", virt, phys, tbl->entries[idx]);
            return res_FAIL;
        }
        tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);
    }

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
    } else tbl = (void *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);

    idx = (virt & hal_mm_pudIdxMask) >> hal_mm_pudShift;
    if (tbl->entries[idx]) {
        printk(RED, BLACK, "mm: map: map1G(): failed to map %p to %p: exist pud entry: %p\n", virt, phys, tbl->entries[idx]);
        return res_FAIL;
    }
    tbl->entries[idx] = phys | attr | 0x80;

    hal_mm_flushTlb();

    return res_SUCC;
}

int hal_mm_unmap(u64 virt) {
    hal_mm_PageTbl *tbl, *subTbl;
    hal_mm_PageTbl *pgd, *pud, *pmd;
    u64 pgdIdx, pudIdx, pmdIdx;

    tbl = virt >= task_krlAddrSt ? mm_dmas_phys2Virt(mm_krlTblPhysAddr) : mm_dmas_phys2Virt(hal_hw_getCR(3));
    pgd = tbl;

    u64 idx = pgdIdx = (virt & hal_mm_pgdIdxMask) >> hal_mm_pgdShift;
    tbl = (hal_mm_PageTbl *)(tbl->entries[idx] & ~0xffful);
    if (tbl == NULL) {
        printk(RED, BLACK, "mm: map: unmap(): failed to unmap %p. pgd entry is NULL\n", virt);
        return res_FAIL;
    }
    pud = tbl;
    
    pudIdx = idx = (virt & hal_mm_pudIdxMask) >> hal_mm_pudShift;
    if (tbl->entries[idx] & 0x80) {
        printk(RED, BLACK, "mm: map: unmap(): failed to unmap %p. part of 1G mapping. entry=%p\n",
            virt, tbl->entries[idx]);
        return res_FAIL;
    }
    tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);

    if (tbl == NULL) {
        printk(RED, BLACK, "mm: map: unmap(): failed to unmap %p. pud entry is NULL\n", virt);
        return res_FAIL;
    }
    pmd = tbl;

    pmdIdx = idx = (virt & hal_mm_pmdIdxMask) >> hal_mm_pmdShift;
    if (tbl->entries[idx] & 0x80) {
        printk(RED, BLACK, "mm: map: unmap(): failed to unmap %p. part of 2M mapping. entry=%p\n",
            virt, tbl->entries[idx]);
        return res_FAIL;
    }
    tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);

    idx = (virt & hal_mm_pldIdxMask) >> hal_mm_pldShift;
    tbl->entries[idx] = 0;

    // check if pld is empty
    int empty = 1;
    for (int i = 0; i < hal_mm_nrPageTblEntries; i++)
        if (tbl->entries[i]) { empty = 0; break; }
    if (empty) {
        mm_map_freeTbl(tbl);
        tbl = NULL;
        pmd->entries[pmdIdx] = 0;
    } else goto endUnmap;

    // check if pmd is empty
    empty = 1;
    for (int i = 0; i < hal_mm_nrPageTblEntries; i++)
        if (pmd->entries[i]) { empty = 0; break; }
    if (empty) {
        mm_map_freeTbl(pmd);
        pmd = NULL;
        pud->entries[pudIdx] = 0;
    } else goto endUnmap;

    // check if pud is empty
    empty = 1;
    for (int i = 0; i < hal_mm_nrPageTblEntries; i++)
        if (pud->entries[i]) { empty = 0; break; }
    if (empty) {
        mm_map_freeTbl(pud);
        pud = NULL;
        pgd->entries[pgdIdx] = 0;
    } else goto endUnmap;

    endUnmap:
    hal_mm_flushTlb();

    return res_SUCC;
}

u64 hal_mm_getMap(u64 virt) {
    hal_mm_PageTbl *tbl, *subTbl;

    tbl = virt >= task_krlAddrSt ? mm_dmas_phys2Virt(mm_krlTblPhysAddr) : mm_dmas_phys2Virt(hal_hw_getCR(3));

    u64 idx = (virt & hal_mm_pgdIdxMask) >> hal_mm_pgdShift;
    tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);
    if (tbl == NULL) return res_FAIL;

    idx = (virt & hal_mm_pudIdxMask) >> hal_mm_pudShift;
    if (~tbl->entries[idx] & 0x1) return 0;
    if (tbl->entries[idx] & 0x80)
        return (tbl->entries[idx] & ~0xffful) | (virt & ((1ull << hal_mm_pudShift) - 1));
    tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);

    idx = (virt & hal_mm_pudIdxMask) >> hal_mm_pudShift;
    if (~tbl->entries[idx] & 0x1) return 0;
    if (tbl->entries[idx] & 0x80)
        return (tbl->entries[idx] & ~0xffful) | (virt & ((1ull<< hal_mm_pudShift) - 1));
    tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);

    idx = (virt & hal_mm_pmdIdxMask) >> hal_mm_pmdShift;
    if (~tbl->entries[idx] & 0x1) return 0;
    if (tbl->entries[idx] & 0x80)
        return (tbl->entries[idx] & ~0xffful) | (virt & ((1ull<< hal_mm_pmdShift) - 1));
    tbl = (hal_mm_PageTbl *)mm_dmas_phys2Virt(tbl->entries[idx] & ~0xffful);

    idx = (virt & hal_mm_pldIdxMask) >> hal_mm_pldShift;
    if (~tbl->entries[idx] & 0x1) return 0;
    return (tbl->entries[idx] & ~0xffful) | (virt & ((1ull << hal_mm_pldShift) - 1));
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
        if (mm_dmas_phys2Virt(NULL) == pudTbl) continue;
        for (int j = 0; j < hal_mm_nrPageTblEntries; j++) if (pudTbl->entries[j]) {
            pmdTbl = mm_dmas_phys2Virt(pudTbl->entries[j] & ~0xffful);
            if (mm_dmas_phys2Virt(NULL) == pmdTbl || (pmdTbl->entries[j] & 0x80ul)) continue;
            for (int k = 0; k < hal_mm_nrPageTblEntries; k++) if (pmdTbl->entries[k]) {
                pldTbl = mm_dmas_phys2Virt(pmdTbl->entries[k] & ~0xffful);
                if (mm_dmas_phys2Virt(NULL) == pldTbl || (pmdTbl->entries[k] & 0x80ul)) continue;
                if (mm_map_freeTbl(pldTbl) == res_FAIL) return res_FAIL;
            }
            if (mm_map_freeTbl(pmdTbl) == res_FAIL) return res_FAIL;
        }
        if (mm_map_freeTbl(pudTbl) == res_FAIL) return res_FAIL;
    }
    return mm_map_freeTbl(pgdTbl);
}