#ifndef __MM_MM_H__
#define __MM_MM_H__

#include <lib/list.h>

struct mm_Page {
	List list;
	u64 physAddr;
	u32 ord;
	u32 attr;
} __attribute__ ((packed));

typedef struct mm_Page mm_Page;
void mm_init();

void mm_allocPage(u64 log2Size);

#endif