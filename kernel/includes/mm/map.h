#ifndef __MM_MAP_H__
#define __MM_MAP_H__

#include <lib/dtypes.h>
#include <lib/atomic.h>
#include <lib/spinlock.h>
#include <hal/mm/map.h>

extern Atomic mm_map_krlTblModiJiff;
extern SpinLock mm_map_krlTblLck;

#ifdef HAL_MM_MAP
int mm_map(u64 virt, u64 phys, u64 attr);
#else
#error no definition of mm_map for this arch!
#endif

#ifdef HAL_MM_UNMAP
int mm_unmap(u64 virt);
#else
#error no definition of mm_unmap for this arch!
#endif

#ifdef HAL_MM_GETMAP
u64 mm_getMap(u64 virt);
#else
#error no definition of hal_mm_getMap for this arch!
#endif

int mm_map_init();

hal_mm_PageTbl *mm_map_allocTbl();

int mm_map_freeTbl(hal_mm_PageTbl *tbl);

int mm_map_syncKrl();
#endif