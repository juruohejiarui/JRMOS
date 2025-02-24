#ifndef __MM_MM_H__
#define __MM_MM_H__

#include <mm/desc.h>

void mm_init();

mm_Page *mm_divPageGrp(mm_Page *grpHdr);
// get the page descriptor for a specific physical address
mm_Page *mm_getDesc(u64 phyAddr);

void *mm_kmalloc(u64 size, u64 attr, void (*desc)(void *));
int mm_kfree(void *addr);


#endif