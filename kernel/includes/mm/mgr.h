// memory management for tasks and corresponding system calls
#ifndef __MM_SHARE_H__
#define __MM_SHARD_H__

#include <lib/dtypes.h>
#include <mm/desc.h>
#include <task/structs.h>

// initialize memory management for thread
void mm_mgr_init(task_MemStruct *mem, u64 tskFlags);

int mm_mgr_free(task_MemStruct *mem);

// find the first map information block s.t. blk->ed > addr
mm_MapBlkInfo *mm_findMap(void *addr);

/// @brief map a memory block and return the virtual address
/// @param attr valid properties:
/// ``mm_Attr_User``: share to user space; 
/// ``mm_Attr_Executable``: executable
/// ``mm_Attr_Writable``: writable
/// @param st restrict the space of allocated virtual address to [st, user_space_end];
/// if st is NULL, it will find a free virtual address randomly
/// @return allocated virtual address, or (void *)-1 if failed
void *mm_mmap(u64 size, u64 attr, void *st, u64 pAddr);

int mm_unmmap(void *addr);

#endif