#ifndef __HAL_TASK_STRUCTS_H__
#define __HAL_TASK_STRUCTS_H__

#include <lib/dtypes.h>
#include <lib/spinlock.h>
#include <hal/mm/map.h>

typedef struct hal_task_ThreadStruct {
	// physical address of pgd
	u64 pgd;
} __attribute__ ((packed)) hal_task_ThreadStruct;

typedef struct hal_task_TaskStruct {
	u64 rip, rsp, fs, gs, rflags;
} __attribute__ ((packed)) hal_task_TaskStruct;
#endif