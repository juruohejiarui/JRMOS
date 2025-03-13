#ifndef __HAL_INIT_INIT_H__
#define __HAL_INIT_INIT_H__

#include <lib/dtypes.h>
#include <hal/interrupt/desc.h>
#include <task/constant.h>

extern u8 hal_init_stk[task_krlStkSize] __attribute__((__section__ (".data.hal_init_stk") ));

extern hal_intr_GdtItem hal_init_gdtTbl[];
extern hal_intr_IdtItem hal_init_idtTbl[];
extern u32 hal_init_tss[26];

void hal_init_init();
void hal_init_initAP();

#endif