#ifndef __HAL_MM_MGR_H__
#define __HAL_MM_MGR_H__

#include <task/structs.h>

void hal_mm_mgr_init(task_MemStruct *mem, u64 tskFlags);

#endif