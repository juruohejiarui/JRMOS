// memory management for tasks and corresponding system calls
#ifndef __MM_SHARE_H__
#define __MM_SHARD_H__

#include <lib/dtypes.h>
#include <mm/desc.h>
#include <task/structs.h>

// initialize memory management for thread
void mm_mgr_init(task_MemStruct *mem, u64 tskFlags);

int mm_mgr_free(task_MemStruct *mem);

// find the first map information block s.t. blk->vAddr <= addr
mm_MapBlkInfo *mm_findMap(void *addr);

/// @brief map a memory block and return the virtual address
/// @param attr valid properties:
/// ``mm_Attr_User``: share to user space; 
/// ``mm_Attr_Executable``: executable
/// ``mm_Attr_Writable``: writable
/// @param vAddr restrict the space of allocated virtual address to [vAddr, user_space_end];
/// if vAddr is NULL, it will find a free virtual address randomly
/// @return allocated virtual address
void *mm_mmap(u64 size, u64 attr, void *vAddr, u64 pAddr);

int mm_unmmap(void *addr);

#endif