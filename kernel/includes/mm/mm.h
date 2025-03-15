#ifndef __MM_MM_H__
#define __MM_MM_H__

#include <mm/desc.h>

int mm_init();

mm_Page *mm_divPageGrp(mm_Page *grpHdr);
// get the page descriptor for a specific physical address
static __always_inline__ mm_Page *mm_getDesc(u64 phyAddr) { return &mm_memStruct.pages[phyAddr >> mm_pageShift]; }
static __always_inline__ u64 mm_getPhyAddr(mm_Page *desc) { return (((u64)desc - (u64)mm_memStruct.pages) / sizeof(mm_Page)) << mm_pageShift; }

mm_Page *mm_init_allocPage(u64 num, u32 attr);

int mm_slab_init();
void mm_slab_debug();

static __always_inline__ int mm_slabRecord_comparator(RBNode *a, RBNode *b) {
    mm_SlabRecord *ta = container(a, mm_SlabRecord, rbNode),
        *tb = container(b, mm_SlabRecord, rbNode);
    return ta->ptr < tb->ptr;
}

RBTree_insert(mm_slabRecord_insert, mm_slabRecord_comparator);

void *mm_kmalloc(u64 size, u32 attr, void (*destructor)(void *));
int mm_kfree(void *addr, u32 attr);
#endif