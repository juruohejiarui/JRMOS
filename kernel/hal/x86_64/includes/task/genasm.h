#ifndef __HAL_TASK_ASM_H__
#define __HAL_TASK_ASM_H__

#define task_TaskStruct_state  0x18
#define task_TaskStruct_signal	0x60
#define task_TaskStruct_hal    0x68
#define task_TaskStruct_thread 0x30
#define hal_task_TaskStruct_rip    0x68
#define hal_task_TaskStruct_rsp    0x70
#define hal_task_TaskStruct_usrRsp 0x90
#define hal_task_TaskStruct_rflags 0x88

#define task_ThreadStruct_hal 0x278
#define hal_task_ThreadStruct_pgd 0x278

#define task_usrStkSize 0x800000
#define task_krlStkSize 0x8000

#define task_state_Running			0
#define task_state_NeedSchedule	0x1
#define task_state_Idle			0x2

#endif
