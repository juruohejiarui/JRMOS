#ifndef __HAL_TASK_ASM_H__
#define __HAL_TASK_ASM_H__

#define task_TaskStruct_state  0x10
#define task_TaskStruct_hal    0x60
#define task_TaskStruct_thread 0x28
#define hal_task_TaskStruct_rip    0x60
#define hal_task_TaskStruct_rsp    0x68
#define hal_task_TaskStruct_rflags 0x80

#define task_ThreadStruct_hal 0x258
#define hal_task_ThreadStruct_pgd 0x258

#define task_state_Running			0
#define task_state_NeedSchedule	0x1
#define task_state_Idle			0x2

#endif
