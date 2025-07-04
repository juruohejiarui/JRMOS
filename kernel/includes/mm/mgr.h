#ifndef __MM_SHARE_H__
#define __MM_SHARD_H__

#include <lib/dtypes.h>
#include <mm/desc.h>
#include <task/structs.h>

/// @todo
// share memory block to user space and return the virtual address in user space
void *mm_shareBlk(void *krlAddr, u64 size, u64 attr);

// allocate user block and return status
int mm_allocBrk(u64 vAddr, u64 size, u64 attr);

int mm_freeBrk(u64 vAddr, u64 size);

// map a memory block to user space
// will not immediately write to the page table and allocate pages
// will return the virtual address of the mapped block
void *mm_mmap(u64 size, u64 attr);

int *mm_unmmap(void *addr);

#define syscall_mm_allocBlk mm_allocBrk

#define syscall_mm_freeBlk mm_freeBrk

#define syscall_mm_mmap mm_mmap

#define syscall_mm_unmmap mm_unmmap

#endif