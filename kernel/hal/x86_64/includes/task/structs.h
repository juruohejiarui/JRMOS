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
	u64 rip, rsp, fs, gs, rflags;
	// user space
	u64 usrRsp, usrStkTop;
	// storage of IA32_GS_BASE and IA32_FS_BASE
	u64 gsBase, fsBase, gsKrlBase;
	hal_intr_TSS tss;
} __attribute__ ((packed)) hal_task_TaskStruct;

#endif