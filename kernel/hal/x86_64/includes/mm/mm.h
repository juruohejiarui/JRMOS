#ifndef __HAL_MM_MM_H__
#define __HAL_MM_MM_H__

#include <lib/dtypes.h>

#define HAL_MM_PAGESIZE
#define hal_mm_pageSize     Page_4KSize
#define hal_mm_pageShift    Page_4KShift

void hal_mm_loadmap();
#endif