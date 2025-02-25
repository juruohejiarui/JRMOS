#ifndef __MM_BUDDY_H__
#define __MM_BUDDY_H__

#include <mm/desc.h>

#define mm_buddy_mxOrd	15

int mm_buddy_init();

mm_Page *mm_allocPages(u64 log2Size, u32 attr);

int mm_freePages(mm_Page *pages);

#endif