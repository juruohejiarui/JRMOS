#ifndef __SOFTIRQ_DESC_H__
#define __SOFTIRQ_DESC_H__

#include <lib/dtypes.h>
#include <lib/atomic.h>
#include <cpu/desc.h>

typedef struct softirq_Desc {
	u32 cpuId, vecId;
	u64 param;
	void (*handler)(u64 param);
} softirq_Desc;

cpu_declarevar(softirq_Desc *[64], softirq_desc);
cpu_declarevar(Atomic, softirq_flag);
cpu_declarevar(u32, softirq_freeCnt);
cpu_declarevar(u32, softirq_useCnt);
cpu_declarevar(u64, softirq_msk);

#endif