#ifndef __MM_MAP_H__
#define __MM_MAP_H__

#include <lib/dtypes.h>
#include <hal/mm/map.h>

#ifdef HAL_MM_MAP
#define mm_map hal_mm_map
#else
#error no definition of hal_mm_map for this arch!
#endif

#ifdef HAL_MM_UNMAP
#define mm_unmap hal_mm_unmap
#else
#error no definition of hal_mm_unmap for this arch!
#endif

extern u64 mm_map_krlTblModiJiff;

int mm_map_init();
hal_mm_PageTbl *mm_map_allocTbl();

int mm_map_freeTbl(hal_mm_PageTbl *tbl);
#endif