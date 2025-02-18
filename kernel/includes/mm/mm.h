#ifndef __MM_MM_H__
#define __MM_MM_H__

#include <mm/desc.h>

void mm_init();

void *mm_kmalloc(u64 size, u64 attr, void (*desc)(void *));
int mm_kfree(void *addr);

#endif