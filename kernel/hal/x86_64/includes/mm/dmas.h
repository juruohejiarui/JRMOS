#ifndef __HAL_MM_DMAS_H__
#define __HAL_MM_DMAS_H__

#include <lib/dtypes.h>

#define HAL_MM_DMAS_MAPSIZE
#define hal_mm_dmas_mapSize Page_1GSize

int hal_mm_dmas_init();

int hal_mm_dmas_map(u64 virt);
#endif