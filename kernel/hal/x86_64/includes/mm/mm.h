#ifndef __HAL_MM_MM_H__
#define __HAL_MM_MM_H__

#include <lib/dtypes.h>

#define hal_mm_PageSize     Page_4KSize
#define hal_mm_PageShift    Page_4KShift

void hal_mm_loadmap();
#endif