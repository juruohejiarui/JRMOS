#include <mm/mgr.h>
#include <mm/mm.h>
#include <mm/buddy.h>
#include <task/api.h>
#include <lib/algorithm.h>

void mm_mgr_init(task_MemStruct *mem, u64 tskFlags) {
    SpinLock_lockMask(&mm_map_krlTblLck);
    mem->krlTblModiJiff.value = 0;
    SpinLock_init(&mem->allocLck);
    mem->allocVirtMem = 0;
    mem->allocMem = 0;

    RBTree_init(&mem->slabRecord, mm_slabRecord_insert, NULL);

    SpinLock_init(&mem->pgTblLck);
    
    hal_mm_mgr_init(&mem->hal, tskFlags);

    SpinLock_unlockMask(&mm_map_krlTblLck);
}

int mm_mgr_free(task_MemStruct *mem) {
    int res = res_SUCC;

    while (mem->mapInfo.mapTr.root) {
        mm_MapBlkInfo *blk = container(mem->mapInfo.mapTr.root, mm_MapBlkInfo, rbNd);
        RBTree_del(&mem->mapInfo.mapTr, &blk->rbNd);
        while (!SafeList_isEmpty(&blk->pgLst)) {
            mm_Page *pg = container(SafeList_delHead(&blk->pgLst), mm_Page, lst);
            mm_freePages(pg);
        }
        u64 mpSz = blk->attr & mm_Attr_Large ? Page_2MSize : Page_4KSize;
        for (void *addr = blk->st; addr < blk->ed; addr += mpSz) {
            if (!mm_getMap((u64)addr)) continue;
            mm_unmap((u64)addr);
        }
    }

    while (mem->slabRecord.root) {
        register mm_SlabRecord *record = container(mem->slabRecord.root, mm_SlabRecord, rbNode);
        res |= mm_kfree(record->ptr, 0);
    }
    return res;
}

static mm_MapBlkInfo *_findMap(task_MemStruct *mem, void *addr) {
    RBNode *cur = mem->mapInfo.mapTr.root;
    mm_MapBlkInfo *blk = container(cur, mm_MapBlkInfo, rbNd), *bst = NULL;
    while (cur != NULL) {
        blk = container(cur, mm_MapBlkInfo, rbNd);
        if (blk->ed > addr) bst = blk, cur = cur->left;
        else cur = cur->right;
    }
}

mm_MapBlkInfo *mm_findMap(void *addr) {
    task_MemStruct *mem = &task_current->thread->mem.mapInfo;
    SpinLock_lockMask(&mem->mapInfo.mapLck);
    register mm_MapBlkInfo *bst = _findMap(mem, addr);
    SpinLock_unlockMask(&mem->mapInfo.mapLck);
    return mem;
}

void *mm_mmap(u64 size, u64 attr, void *st, u64 pAddr) {
    task_MemStruct *mem = &task_current->thread->mem.mapInfo;
    // align to 4K
    st = (u64)downAlign((u64)st, Page_4KSize);
    size = upAlign(size, Page_4KSize);
    SpinLock_lockMask(&mem->mapInfo.mapLck);
    mm_MapBlkInfo *blk = _findMap(mem, st);
    for (mm_MapBlkInfo *blk = _findMap(mem, st); ; st = blk->ed, blk = container(blk->lstNd.next, mm_MapBlkInfo, lstNd)) {
        if (st + size > mm_usrAddrEd)
            return (void *)-1;
        if (blk->st >= st + size) {
            
    }
    SpinLock_unlockMask(&mem->mapInfo.mapLck);
}