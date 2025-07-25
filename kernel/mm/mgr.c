#include <mm/mgr.h>
#include <mm/mm.h>
#include <mm/buddy.h>
#include <hal/mm/mgr.h>
#include <task/api.h>
#include <lib/algorithm.h>
#include <screen/screen.h>

__always_inline__ int mm_MapBlkInfo_cmp(RBNode *a, RBNode *b) {
    return container(a, mm_MapBlkInfo, rbNd)->ed < container(b, mm_MapBlkInfo, rbNd)->ed;
}

RBTree_insertDef(mm_MapBlkInfo_insert, mm_MapBlkInfo_cmp);

void mm_mgr_init(task_MemStruct *mem, u64 tskFlags) {
    SpinLock_lockMask(&mm_map_krlTblLck);
    mem->krlTblModiJiff.value = 0;
    SpinLock_init(&mem->allocLck);
    mem->allocVirtMem = 0;
    mem->allocMem = 0;

    RBTree_init(&mem->slabRecord, mm_slabRecord_insert, NULL);
    RBTree_init(&mem->mapInfo.mapTr, mm_MapBlkInfo_insert, NULL);
    
    SpinLock_init(&mem->pgTblLck);
    SpinLock_init(&mem->mapInfo.mapLck);
    
    hal_mm_mgr_init(mem, tskFlags);

    SpinLock_unlockMask(&mm_map_krlTblLck);
}

// delete map table, release pages and delete map information block
static int _delMap(mm_MapBlkInfo *blk) {
    int res = res_SUCC;
    while (!SafeList_isEmpty(&blk->pgLst)) {
        mm_Page *pg = container(SafeList_delHead(&blk->pgLst), mm_Page, lst);
        res |= mm_freePages(pg);
    }
    u64 mpSz = blk->attr & mm_Attr_Large ? Page_2MSize : Page_4KSize;
    for (void *addr = blk->st; addr < blk->ed; addr += mpSz) {
        if (!mm_getMap((u64)addr)) continue;
        res |= mm_unmap((u64)addr);
    }
    return res | mm_kfree(blk, mm_Attr_Shared);
}
int mm_mgr_free(task_MemStruct *mem) {
    int res = res_SUCC;

    while (mem->mapInfo.mapTr.root) {
        mm_MapBlkInfo *blk = container(mem->mapInfo.mapTr.root, mm_MapBlkInfo, rbNd);
        RBTree_del(&mem->mapInfo.mapTr, &blk->rbNd);
        _delMap(blk);
    }

    while (mem->slabRecord.root) {
        register mm_SlabRecord *record = container(mem->slabRecord.root, mm_SlabRecord, rbNode);
        res |= mm_kfree(record->ptr, 0);
    }
    return res;
}

// find the block with minimum ``blk->ed`` such that ``blk->ed > addr``
static mm_MapBlkInfo *_findMap(mm_MapInfo *mem, void *addr) {
    RBNode *cur = mem->mapTr.root;
    mm_MapBlkInfo *blk = container(cur, mm_MapBlkInfo, rbNd), *bst = NULL;
    while (cur != NULL) {
        blk = container(cur, mm_MapBlkInfo, rbNd);
        if (blk->ed > addr) bst = blk, cur = cur->left;
        else cur = cur->right;
    }
    return bst;
}

mm_MapBlkInfo *mm_findMap(void *addr) {
    mm_MapInfo *mem = &task_current->thread->mem.mapInfo;
    SpinLock_lockMask(&mem->mapLck);
    register mm_MapBlkInfo *bst = _findMap(mem, addr);
    SpinLock_unlockMask(&mem->mapLck);
    return bst;
}

__always_inline__ void *_align(void *st, u64 attr) { return (void *)upAlign((u64)st, (attr & mm_Attr_Large ? Page_2MSize : Page_4KSize)); }
void *mm_mmap(u64 size, u64 attr, void *st, u64 pAddr) {
    mm_MapInfo *mem = &task_current->thread->mem.mapInfo;
    // align to 4K
    st = _align(st, attr);
    size = _align(size, attr);
    SpinLock_lockMask(&mem->mapLck);
    mm_MapBlkInfo *blk = _findMap(mem, st), *newBlk;
    for (; ; st = blk->ed, blk = _findMap(mem, st)) {
        if (st + size > (void *)mm_usrAddrEd)
            st = (void *)mm_usrAddrEd;
        if (blk == NULL || blk->st >= st + size) {
            // block [st, st + size] is valid
            break;
        }
        st = _align(blk->ed, attr);
    }
    if (st == (void *)mm_usrAddrEd) {
        SpinLock_unlockMask(&mem->mapLck);
        return (void *)-1;
    }
    newBlk = mm_kmalloc(sizeof(mm_MapBlkInfo), mm_Attr_Shared, NULL);
    newBlk->st = st;
    newBlk->ed = st + size;
    SafeList_init(&newBlk->pgLst);
    newBlk->attr = attr;
    newBlk->pAddr = pAddr;

    // add to map information

    RBTree_ins(&mem->mapTr, &newBlk->rbNd);
    
    SpinLock_unlockMask(&mem->mapLck);

    printk(screen_log, "mm: #%d: mem_mmap: addr:%p\n", task_current->pid, st);

    return st;
}

int mm_munmap(void *addr, u64 size) {
    mm_MapInfo *mem = &task_current->thread->mem.mapInfo;
    void *addrEd = addr + size;
    int res = res_SUCC;
    SpinLock_lockMask(&mem->mapLck);
    mm_MapBlkInfo *blk = _findMap(mem, addr);
    if (blk == NULL || blk->st != addr || blk->ed != addrEd) {
        printk(screen_err, "mem: #%5d: mem_unmap: addr:%#018lx size:%#018lx\n", task_current->pid, addr, size);
        goto mm_munmap_failed;
    }
    // delete node from rbtree
    RBTree_del(&mem->mapTr, &blk->rbNd);
    SpinLock_unlockMask(&mem->mapLck);
    // delete map
    return _delMap(blk);
    mm_munmap_failed:
    SpinLock_unlockMask(&mem->mapLck);
    return res_FAIL;
}