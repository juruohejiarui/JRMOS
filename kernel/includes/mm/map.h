#ifndef __MM_MAP_H__
#define __MM_MAP_H__

#include <lib/dtypes.h>
#include <lib/atomic.h>
#include <lib/spinlock.h>
#include <hal/mm/map.h>

extern Atomic mm_map_krlTblModiJiff;
extern SpinLock mm_map_krlTblLck;
extern SpinLock mm_map_dbgLck;

void mm_map_dbgMap(u64 virt);

int mm_map(u64 virt, u64 phys, u64 attr);

// unmap a virtual memory space start from virt
// return the size of this virtual memory space
int mm_unmap(u64 virt);

u64 mm_getMap(u64 virt);

int mm_map_init();

void mm_map_dbg(int detail);

hal_mm_PageTbl *mm_map_allocTbl();

int mm_map_freeTbl(hal_mm_PageTbl *tbl);

int mm_map_syncKrl();
#endif