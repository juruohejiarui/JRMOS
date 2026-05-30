#ifndef __HAL_SIMD_DEFINE_H__
#define __HAL_SIMD_DEFINE_H__

#include <cpu/api.h>

typedef struct hal_simd_SaveBlk {
    u8 legacyArea[512];
	union {
        struct {
            u64 xstateBv;
            u64 xcompBv;
        };
        u8 hdrArea[64];
    };
    u8 extArea[];
} hal_simd_SaveBlk;

cpu_declarevar(u64, hal_simd_flag);
cpu_declarevar(u64, hal_simd_blkSz);

#endif