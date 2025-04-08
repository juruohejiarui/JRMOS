#include <mm/map.h>
#include <mm/buddy.h>
#include <mm/mm.h>
#include <mm/dmas.h>
#include <hal/mm/map.h>
#include <screen/screen.h>
#include <lib/algorithm.h>
#include <lib/spinlock.h>
#include <lib/bit.h>
#include <interrupt/api.h>
#include <task/api.h>

#define nrTblCacheShift 10
#define nrTblCache (1ull << nrTblCacheShift)

#define pgNumPerTbl (upAlign(sizeof(hal_mm_PageTbl), mm_pageSize) >> mm_pageShift)

Atomic mm_map_krlTblModiJiff;

static mm_Page *_tblGrpCache[nrTblCache << 1];
static u64 _allocLg2;

static u64 _nrTblGrp, _nrTbl;

static SpinLock _cacheLck;
SpinLock mm_map_krlTblLck;
SpinLock mm_map_dbgLck;

static int _initCache() {
    _nrTblGrp = 1;
    _nrTbl = (1ull << nrTblCacheShift);
    return (_tblGrpCache[0] = mm_allocPages(nrTblCacheShift + _allocLg2, mm_Attr_Shared)) ? res_SUCC : res_FAIL;
    
}

int mm_map_init() {
	mm_map_krlTblModiJiff.value = 0;
    _allocLg2 = bit_ffs64(pgNumPerTbl) - 1;
    SpinLock_init(&_cacheLck);
    SpinLock_init(&mm_map_krlTblLck);
    printk(WHITE, BLACK, "mm: map: alloc lg2=%d\n", _allocLg2);
    return _initCache();
}

hal_mm_PageTbl *mm_map_allocTbl() {
    int intrState = intr_state();
    if (intrState) intr_mask();
    SpinLock_lock(&_cacheLck);

    if (!_nrTbl) _initCache();
    while (_tblGrpCache[_nrTblGrp - 1]->ord > _allocLg2)
        _tblGrpCache[_nrTblGrp] = mm_divPageGrp(_tblGrpCache[_nrTblGrp - 1]),
        _nrTblGrp++;
    mm_Page *page = _tblGrpCache[--_nrTblGrp];
    _nrTbl--;

    SpinLock_unlock(&_cacheLck);
    if (intrState) intr_unmask();

    page->attr |= mm_Attr_MMU;

    hal_mm_PageTbl *tbl = mm_dmas_phys2Virt(mm_getPhyAddr(page));
    memset(tbl, 0, sizeof(hal_mm_PageTbl));
    return tbl;
}

void mm_map_dbg(int detail) {
    int intrState = intr_state();
    if (intrState) intr_mask();
    SpinLock_lock(&_cacheLck);


    printk(WHITE, BLACK, "mm:map:nrTbl:%d nrTblGrp:%d ", _nrTbl, _nrTblGrp);
    if (!detail) {
        SpinLock_unlock(&_cacheLck);
        if (intrState) intr_unmask();
        return ;
    }
    printk(WHITE, BLACK, "\n");
    for (int i = 0; i < _nrTblGrp; i++)
        printk(WHITE, BLACK, "%#011lx %2d%c",
            (u64)_tblGrpCache[i] & 0xffffffffful, _tblGrpCache[i]->ord,
            (i % 12 == 11 ? '\n' : ' '));
    if (_nrTblGrp % 12 != 0) printk(WHITE, BLACK, "\n");

    SpinLock_unlock(&_cacheLck);
    if (intrState) intr_unmask();
}

int mm_map_freeTbl(hal_mm_PageTbl *tbl) {
    mm_Page *pg = mm_getDesc(mm_dmas_virt2Phys(tbl));
    if (~pg->attr & (mm_Attr_MMU | mm_Attr_HeadPage)) {
        printk(RED, BLACK, "mm: failed to free page table %#018lx. Invalid table pointer.\n", tbl);
        while (1) ;
        return res_FAIL;
    }
    int intrState = intr_state();
    if (intrState) intr_mask();
    SpinLock_lock(&_cacheLck);

    // directly free the table if there is enough cache
    if (_nrTbl >= nrTblCache) {
        SpinLock_unlock(&_cacheLck);
        if (intrState) intr_unmask();
        return mm_freePages(pg);
    }
    _tblGrpCache[_nrTblGrp++] = pg;
    _nrTbl++;
    pg->attr ^= mm_Attr_MMU;

    // release the lock and interrupt
    SpinLock_unlock(&_cacheLck);
    if (intrState) intr_unmask();

    return res_SUCC;
}

int mm_map_syncKrl() {
    if (task_current->thread->krlTblModiJiff.value != mm_map_krlTblModiJiff.value) {
        SpinLock_lock(&mm_map_krlTblLck);
        SpinLock_lock(&task_current->thread->pgTblLck);

        task_current->thread->krlTblModiJiff.value = mm_map_krlTblModiJiff.value;

	    int res = hal_mm_map_syncKrl();

        SpinLock_unlock(&task_current->thread->pgTblLck);
        SpinLock_unlock(&mm_map_krlTblLck);

        return res;
    }
    return res_SUCC;
}

void mm_map_dbgMap(u64 virt) {
    SpinLock_lock(&mm_map_dbgLck);
    hal_mm_map_dbgMap(virt);
    SpinLock_unlock(&mm_map_dbgLck);
}

int mm_map(u64 virt, u64 phys, u64 attr) {
    if (virt >= task_krlAddrSt) {
        SpinLock_lock(&mm_map_krlTblLck);
        hal_Atomic_inc(&mm_map_krlTblModiJiff);
    }
    else SpinLock_lock(&task_current->thread->pgTblLck);

    int res = hal_mm_map(virt, phys, attr);

    if (virt >= task_krlAddrSt) SpinLock_unlock(&mm_map_krlTblLck);
    else SpinLock_unlock(&task_current->thread->pgTblLck);

    return res;
}

int mm_unmap(u64 virt) {
    if (virt >= task_krlAddrSt) {
        SpinLock_lock(&mm_map_krlTblLck);
        hal_Atomic_inc(&mm_map_krlTblModiJiff);
    }
    else SpinLock_lock(&task_current->thread->pgTblLck);

    int res = hal_mm_unmap(virt);
    
    if (virt >= task_krlAddrSt) SpinLock_unlock(&mm_map_krlTblLck);
    else SpinLock_unlock(&task_current->thread->pgTblLck);

    return res;
}

u64 mm_getMap(u64 virt) {
    if (virt >= task_krlAddrSt) SpinLock_lock(&mm_map_krlTblLck);
    else SpinLock_lock(&task_current->thread->pgTblLck);

    u64 phyAddr = hal_mm_getMap(virt);
    
    if (virt >= task_krlAddrSt) SpinLock_unlock(&mm_map_krlTblLck);
    else SpinLock_unlock(&task_current->thread->pgTblLck);

    return phyAddr;
}