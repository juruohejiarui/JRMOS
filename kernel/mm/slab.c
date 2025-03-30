#include <mm/mm.h>
#include <mm/dmas.h>
#include <mm/buddy.h>
#include <lib/algorithm.h>
#include <lib/bit.h>
#include <lib/spinlock.h>
#include <lib/string.h>
#include <task/api.h>
#include <screen/screen.h>
#include <interrupt/api.h>

#define alloc2MPage() mm_allocPages(Page_2MShift - mm_pageShift, mm_Attr_Shared)

static SpinLock _SlabLck;

typedef struct SlabBlk {
    List list;
    mm_Page *page;
    void *addr;
    u64 usingCnt, freeCnt;

    u64 colLen, colCnt;
    u64 colMap[0];
} SlabBlk;

typedef struct Slab {
    u64 size, szShift;
    u64 usingCnt, freeCnt;
    List blkList;
} Slab;

static Slab _slab[mm_slab_mxSizeShift - mm_slab_mnSizeShift + 1];

static __always_inline__ int _addRecord(void *addr, u64 size, void (*destructor)(void *)) {
    mm_SlabRecord *record = mm_kmalloc(sizeof(mm_SlabRecord), mm_Attr_Shared, NULL);
    if (!record) return res_FAIL;
    record->destructor = destructor;
    record->ptr = addr;
    record->size = size;
    RBTree_ins(&task_current->thread->slabRecord, &record->rbNode);
    return res_SUCC;
}

int _delRecord(void *addr) {
    RBNode *node = task_current->thread->slabRecord.root;
    mm_SlabRecord *record = NULL;
    while (node) {
        record = container(node, mm_SlabRecord, rbNode);
        if (record->ptr == addr) break;
        else if (record->ptr < addr) node = node->right;
        else node = node->left;
    }
    if (!record) {
        printk(RED, BLACK, "mm: slab: delRecord(): failed to find record of private address %#018lx\n", addr);
        return res_FAIL;
    }
    RBTree_del(&task_current->thread->slabRecord, node);
    return mm_kfree(record, mm_Attr_Shared);
}

int mm_slab_init() {
    SpinLock_init(&_SlabLck);
    mm_Page *page;
    {
        u64 size = (mm_slab_mxSizeShift - mm_slab_mnSizeShift + 1) * sizeof(SlabBlk);
        for (int i = 0; i < mm_slab_mxSizeShift - mm_slab_mnSizeShift + 1; i++) {
            _slab[i].szShift = i + mm_slab_mnSizeShift;
            _slab[i].size = (1ull<< (i + mm_slab_mnSizeShift));
            size += upAlign(Page_2MSize / _slab[i].size, 64) / 64 * sizeof(u64);
        }
        size = upAlign(size, mm_pageSize);
        printk(WHITE, BLACK, "mm: slab: initialization memory %ld\n", size);
        u64 log2Size = 0;
        while ((1ull<< log2Size) < (size >> mm_pageShift)) log2Size++;
        page = mm_allocPages(log2Size, mm_Attr_Shared);
            
        if (page == NULL) {
            printk(RED, BLACK, "mm: slab: failed to alloc 2^%d pages for initialization.\n", log2Size);
            return res_FAIL;
        }   
    }
    u64 addr = (u64)mm_dmas_phys2Virt(mm_getPhyAddr(page));
    for (int i = 0; i < mm_slab_mxSizeShift - mm_slab_mnSizeShift + 1; i++) {
        Slab *slab = &_slab[i];
        SlabBlk *blk = (SlabBlk *)addr;
        List_init(&slab->blkList);

        blk->usingCnt = 0, blk->freeCnt = Page_2MSize >> _slab[i].szShift;
        blk->page = alloc2MPage();
        if (blk->page == NULL) {
            printk(RED, BLACK, "mm: slab: failed to alloc 2M pages for slab #%d\n", i);
            return res_FAIL;
        }
        blk->addr = mm_dmas_phys2Virt(mm_getPhyAddr(blk->page));
        blk->colCnt = blk->freeCnt;
        blk->colLen = upAlign(blk->colCnt, 64) / 64;
        memset(blk->colMap, 0xff, blk->colLen * sizeof(u64));
        addr += sizeof(SlabBlk) + blk->colLen * sizeof(u64);
        for (u64 j = 0; j < blk->colCnt; j++) bit_set0(&blk->colMap[j >> 6], j & 63);

        List_insBefore(&blk->list, &slab->blkList);
        slab->freeCnt = blk->freeCnt;
        slab->usingCnt = blk->usingCnt;
    }
    return res_SUCC;
}

void mm_slab_debug() {
    printk(WHITE, BLACK, "mm: slab:\n");
    for (int i = 0; i < mm_slab_mxSizeShift - mm_slab_mnSizeShift + 1; i++) {
        Slab *slab = &_slab[i];
        printk(YELLOW, BLACK, "[%2d] size=%lu using=%ld free=%ld\n", i, slab->size, slab->usingCnt, slab->freeCnt);
        for (List *list = slab->blkList.next; list != &slab->blkList; list = list->next) {
            SlabBlk *blk = container(list, SlabBlk, list);
            printk(WHITE, BLACK, "\t(%p): using=%5d free=%5d colMap[0]=%#018lx addr=%#018lx\n",
                blk, blk->usingCnt, blk->freeCnt, blk->colMap[0], blk->addr);
        }
    }
}

// inner of kmalloc
static void *_kmalloc(u64 size);
static int _kfree(void *addr);

static __always_inline__ int _newSlabBlk(int sizeId) {
    mm_Page *pages = alloc2MPage();
    if (pages == NULL) {
        printk(RED, BLACK, "mm: slab: newSlabBlk(): failed to allocate 2M page for a new slab.\n");
        return res_FAIL;
    }
    u64 structSz = 0; void *addr = mm_dmas_phys2Virt(mm_getPhyAddr(pages));
    SlabBlk *blk;
    if (0 <= sizeId && sizeId < 5) {
        structSz = sizeof(SlabBlk) + upAlign(Page_2MSize >> _slab[sizeId].szShift, 64) / 64 * sizeof(u64);
        blk = (SlabBlk *)((u64)addr + Page_2MSize - structSz);
        blk->usingCnt = 0;
        blk->freeCnt = (Page_2MSize - structSz) >> _slab[sizeId].szShift;
        
        blk->colCnt = blk->freeCnt;
        blk->colLen = upAlign(blk->colCnt, 64) / 64;
    } else {
        u64 colCnt = Page_2MSize >> _slab[sizeId].szShift, colLen = upAlign(colCnt, 64) / 64;
        blk = _kmalloc(sizeof(SlabBlk) + colLen * sizeof(u64));
        if (blk == NULL) {
            printk(RED, BLACK, "mm: slab: newSlabBlk(): failed to allocate slab block with size=%d\n", sizeof(SlabBlk) + colLen * sizeof(u64));
            return res_FAIL;
        }
        blk->usingCnt = 0;

        blk->freeCnt = blk->colCnt = colCnt;
        blk->colLen = colLen;
    }
    blk->addr = addr;
    blk->page = pages;

    memset(blk->colMap, 0xff, blk->colLen * sizeof(u64));
    for (int j = 0; j < blk->colCnt; j++) bit_set0(blk->colMap + (j >> 6), j & 63);

    List_init(&blk->list);
    List_insBehind(&blk->list, &_slab[sizeId].blkList);
    _slab[sizeId].freeCnt += blk->freeCnt;
    return res_SUCC;
}

static __always_inline__ int _freeSlabBlk(int sizeId, SlabBlk *blk) {
    List_del(&blk->list);
    _slab[sizeId].freeCnt -= blk->freeCnt;
    if (0 <= sizeId && sizeId < 5)
        return mm_freePages(blk->page);
    else {
        if (mm_freePages(blk->page) == res_FAIL) return res_FAIL;
        return _kfree(blk);
    }
}

static void *_kmalloc(u64 size) {
    int id = 0;
    while (_slab[id].size < size) id++;
    SlabBlk *blk = NULL;
    if (_slab[id].freeCnt > 0) {
        for (List *list = _slab[id].blkList.next; list != &_slab[id].blkList; list = list->next) {
            blk = container(list, SlabBlk, list);
            if (blk->freeCnt > 0) break;
        }
        // move this block to the front the list
        if (&blk->list != _slab[id].blkList.next) {
            List_del(&blk->list);
            List_insBehind(&blk->list, &_slab[id].blkList);
        }
    } else {
        if (_newSlabBlk(id) == res_FAIL) {
            printk(RED, BLACK, "mm: slab: kmalloc(): failed to make new slab block with size=%ld\n", _slab[id].size);
            return NULL;
        }
        blk = container(_slab[id].blkList.next, SlabBlk, list);
    }

    for (u64 j = 0; j < blk->colCnt; j++) {
        if (blk->colMap[j >> 6] == (~0x0ull)) { j += 63; continue; }
        if (blk->colMap[j >> 6] & (1ull << (j & 63))) continue;
        bit_set1(&blk->colMap[j >> 6], j & 63);
        blk->usingCnt++, blk->freeCnt--;
        _slab[id].usingCnt++, _slab[id].freeCnt--;
        return blk->addr + (j << _slab[id].szShift);
    }
    
    while (1) ;
    return NULL;
}

static int _kfree(void *addr) {
    int sizeId = 0, find = 0;
    SlabBlk *blk;
    for (sizeId = 0; sizeId < mm_slab_mxSizeShift - mm_slab_mnSizeShift + 1; sizeId++) {
        for (List *list = _slab[sizeId].blkList.next; list != &_slab[sizeId].blkList; list = list->next) {
            blk = container(list, SlabBlk, list);
            if (blk->addr <= addr && addr < blk->addr + (blk->colCnt << _slab[sizeId].szShift)) {
                find = 1;
                break;
            }
        }
        if (find) break;
    }
    if (!find) {
        printk(RED, BLACK, "mm: slab: kfree(): invalid address %#018lx\n", addr);
        return res_FAIL;
    }
    u64 idx = ((u64)addr - (u64)blk->addr) >> _slab[sizeId].szShift;
    bit_set0(blk->colMap + (idx >> 6), idx & 63);
    blk->freeCnt++, blk->usingCnt--;
    _slab[sizeId].freeCnt++, _slab[sizeId].usingCnt--;
    if (blk->usingCnt == 0 && _slab[sizeId].freeCnt >= blk->colCnt * 3 / 2
         && !(_slab[sizeId].blkList.next == &blk->list && _slab[sizeId].blkList.prev == &blk->list))
        return _freeSlabBlk(sizeId, blk);
    return res_SUCC;
}

void *mm_kmalloc(u64 size, u32 attr, void (*destructor)(void *)) {
    if (size > mm_slab_mxSize) {
        printk(RED, BLACK, "mm: slab: kmalloc(): failed to allocate a memory block with size %ld. Too large\n", size);
        return NULL;
    }
    int intrState = intr_state();
    intr_mask();
    SpinLock_lock(&_SlabLck);
    void *addr = _kmalloc(size);
    SpinLock_unlock(&_SlabLck);
    if (!intrState) intr_unmask();
    if (addr && (~attr & mm_Attr_Shared)) {
        int i;
        for (i = 0; i < mm_slab_mxSizeShift - mm_slab_mnSizeShift + 1; i++)
            if (_slab[i].size >= size) break;
        if (_addRecord(addr, i, destructor) == res_FAIL) {
            mm_kfree(addr, attr | mm_Attr_Shared);
            return NULL;
        }
    }
    return addr;
}
int mm_kfree(void *addr, u32 attr) {
    if (~attr & mm_Attr_Shared) {
        if (_delRecord(addr) == res_FAIL) {
            printk(RED, BLACK, "mm: slab: kfree(): failed to delete record of private address %#018lx.\n", addr);
            return res_FAIL;
        }
    }
    int intrState = intr_state();
    intr_mask();
    SpinLock_lock(&_SlabLck);

    int res = _kfree(addr);

    SpinLock_unlock(&_SlabLck);
    if (!intrState) intr_unmask();
    return res;
}