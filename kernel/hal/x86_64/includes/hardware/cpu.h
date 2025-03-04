#ifndef __HAL_HW_CPU_H__
#define __HAL_HW_CPU_H__

#include <lib/dtypes.h>

#define hal_hw_cpu_mxNum	128

void hal_hw_cpu_cpuid(u32 mop, u32 sop, u32 *a, u32 *b, u32 *c, u32 *d);
#endif