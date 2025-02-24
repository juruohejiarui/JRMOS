#include <mm/map.h>
#include <mm/buddy.h>
#include <mm/mm.h>
#include <mm/dmas.h>
#include <hal/mm/map.h>
#include <screen/screen.h>

#define nrTblCacheShift 10
#define nrTblCache (1ul << nrTblCacheShift)

static mm_Page *_tblGrpCache[nrTblCache];

static u64 nrTblGrp, nrTbl;

static void _initCache() {
    nrTblGrp = 1;
    nrTbl = (1ul << nrTblCacheShift);
    _tblGrpCache[0] = mm_allocPages(nrTblCacheShift, mm_Attr_Shared);
}

void mm_map_initCache() {
    nrTblGrp = 1;
    _initCache();
}
hal_mm_PageTbl *mm_map_allocTbl() {
    if (!nrTblGrp) _initCache();
    while (_tblGrpCache[nrTblGrp - 1]->ord)
        _tblGrpCache[nrTblGrp++] = mm_divPageGrp(_tblGrpCache[nrTblGrp - 1]);
    mm_Page *page = _tblGrpCache[--nrTblGrp];
    page->attr |= mm_Attr_Allocated;
    return mm_dmas_phys2Virt(page->physAddr);
}

int mm_map_freeTbl(hal_mm_PageTbl *tbl) {
    mm_Page *pg = mm_getDesc(mm_dmas_virt2Phys(tbl));
    if ((~pg->attr & mm_Attr_Allocated) || ~pg->attr & mm_Attr_HeadPage) {
        printk(RED, BLACK, "mm: failed to free page table %#018lx. Invalid table pointer.\n", tbl);
        return res_FAIL;
    }
    // directly free the table if there is enough cache
    if (nrTbl == nrTblCache) return mm_freePages(pg);
    _tblGrpCache[nrTblGrp++] = pg;
    nrTbl++;
    return res_SUCC;
}

