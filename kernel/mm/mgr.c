#include <mm/mgr.h>
#include <mm/mm.h>
#include <mm/buddy.h>

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
        for (void *addr = blk->vAddr; addr < blk->vAddr + blk->size; addr += mpSz) {
            
        }
    }

    while (mem->slabRecord.root) {
        register mm_SlabRecord *record = container(mem->slabRecord.root, mm_SlabRecord, rbNode);
        res |= mm_kfree(record->ptr, 0);
    }
    return res;
}