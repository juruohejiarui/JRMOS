#ifndef __HAL_INIT_INIT_H__
#define __HAL_INIT_INIT_H__

#include <lib/dtypes.h>
#include <task/constant.h>

extern u8 hal_init_stk[task_krlStkSize] __attribute__((__section__ (".data.Init_stack") ));

void hal_init_init();

#endif