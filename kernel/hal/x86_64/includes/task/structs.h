#ifndef __HAL_TASK_STRUCTS_H__
#define __HAL_TASK_STRUCTS_H__

#include <lib/dtypes.h>
#include <lib/spinlock.h>
#include <hal/interrupt/desc.h>
#include <hal/mm/map.h>

typedef struct hal_task_ThreadStruct {
	// physical address of pgd
	u64 pgd;
	hal_mm_PageTbl *pgTbl;
} __attribute__ ((packed)) hal_task_ThreadStruct;

typedef struct hal_task_TaskStruct {
	u64 rip, rsp, rsp2, fs, gs, rflags;
	hal_intr_TSS tss;
} __attribute__ ((packed)) hal_task_TaskStruct;

typedef struct hal_task_UsrStruct {
	// fs and gs of kernel and user
	u64 krlFs, krlGs, usrFs, usrGs;
} __attribute__ ((packed)) hal_task_UsrStruct;

#endif