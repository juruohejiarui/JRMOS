#ifndef __MM_MM_H__
#define __MM_MM_H__

#include <mm/desc.h>

int mm_init();

void mm_dbg();

// divide the page group and return the right part of the original group
// the group should not be in any list when calling this function
mm_Page *mm_divPageGrp(mm_Page *grpHdr);
// get the page descriptor for a specific physical address
mm_Page *mm_getDesc(u64 phyAddr);
u64 mm_getPhyAddr(mm_Page *desc);

mm_Page *mm_init_allocPage(u64 num, u32 attr);

int mm_slab_init();
void mm_slab_dbg();

__always_inline__ int mm_slabRecord_comparator(RBNode *a, RBNode *b) {
    mm_SlabRecord *ta = container(a, mm_SlabRecord, rbNd),
        *tb = container(b, mm_SlabRecord, rbNd);
    return ta->ptr < tb->ptr;
}

RBTree_insertDef(mm_slabRecord_insert, mm_slabRecord_comparator);

typedef struct task_MemStruct task_MemStruct;
int mm_slab_clear(task_MemStruct *mem);

void *mm_kmalloc(u64 size, u32 attr, void (*destructor)(void *));
int mm_kfree(void *addr, u32 attr);
#endif