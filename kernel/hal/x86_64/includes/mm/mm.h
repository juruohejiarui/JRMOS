#ifndef __HAL_MM_MM_H__
#define __HAL_MM_MM_H__

#include <lib/dtypes.h>

#define HAL_MM_PAGESIZE
#define hal_mm_pageSize     Page_4KSize
#define hal_mm_pageShift    Page_4KShift

#define HAL_MM_KRLTBLPHYSADDR
#define hal_mm_krlTblPhysAddr 0x101000

#define hal_mm_segment_KrlCode  0x8
#define hal_mm_segment_KrlData  0x10
#define hal_mm_segment_UsrData  0x30
#define hal_mm_segment_UsrCode  0x38



void hal_mm_loadmap();
#endif