#ifndef __SOFTIRQ_DESC_H__
#define __SOFTIRQ_DESC_H__

#include <lib/dtypes.h>

typedef struct softirq_Desc {
	u32 cpuId, vecId;
	u64 param;
	void (*handler)(u64 param);
} softirq_Desc;

#endif