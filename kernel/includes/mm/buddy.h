#ifndef __MM_BUDDY_H__
#define __MM_BUDDY_H__

#include <mm/desc.h>

#define mm_buddy_mxOrd	15

#define mm_buddy_isRight(page)      ((page)->buddyId & 1)
#define mm_buddy_parent(buddyId)    ((buddyId) >> 1)
#define mm_buddy_lson(buddyId)      ((buddyId) << 1)
#define mm_buddy_rson(buddyId)      ((buddyId) << 1 | 1)

int mm_buddy_init();

void mm_buddy_debug();

mm_Page *mm_allocPages(u64 log2Size, u32 attr);

int mm_freePages(mm_Page *pages);

#endif