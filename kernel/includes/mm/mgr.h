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

/// allocate a virtual memory block ``[a, b] (a >= st)`` on user space and return the virtual address.
///
/// default ``map_blk_sz``=4K and can be set to 2M by setting ``mm_Attr_Large`` attribute.
/// ``a`` and ``b`` will be aligned to ``map_blk_sz``.
/// 
/// Page table entries will be allocated on ``PageFault`` exception handler.
/// Will not allocate physical memory immediately.
/// @param size minimum size of virtual memory block, actual size is by aligning it to ``map_blk_sz``.
/// @param attr valid properties:
///
/// - ``mm_Attr_User``: share to user space; 
///
/// - ``mm_Attr_Executable``: executable
///
/// - ``mm_Attr_Writable``: writable
/// 
///	- ``mm_Attr_Large``: large map block, set ``map_blk_sz<-2M``
/// 
/// - ``mm_Attr_Fixed``: fixed map block, map to a fixed physical memory space
/// 
/// Other attribute will be ignored
/// @param st restrict the space of allocated virtual address to ``[st, user_space_end]``;
/// @param pAddr 
/// restrict the pAddr 
/// 
/// - if ``mm_Attr_Fixed`` is not set, then this block is random mapping block
/// and physical memory will be allocated on ``PageFault`` exception handler,
/// which might NOT be contiguous
/// 
/// - otherwise, this map block is forced to map to physical memory ``[pAddr, pAddr + size]``
/// @return allocated virtual address ``a``, or ``(void *)-1`` if failed
void *mm_mmap(u64 size, u64 attr, void *st, u64 pAddr);

/// @brief currently only support for deletion of entire map block
/// @param addr 
/// @param size 
/// @return 
int mm_munmap(void *addr, u64 size);

// share the memory block to task with
void *mm_mshare(void *addr, u64 size, u64 pid) {

}

#endif