#ifndef __HAL_TASK_ASM_H__
#define __HAL_TASK_ASM_H__

#define task_Thread_state  0x18
#define task_Thread_signal	0x80
#define task_Thread_signalHandle 0x88
#define task_Thread_hal    0x90
#define task_Thread_thread 0x30
#define hal_task_Thread_rip    0x90
#define hal_task_Thread_rsp    0x98
#define hal_task_Thread_usrRsp 0xb8
#define hal_task_Thread_rflags 0xb0

#define task_Process_hal 0x538
#define hal_task_Process_pgd 0x80

#define task_usrStkSize 0x1000000
#define task_krlStkSize 0x8000

#define task_state_Running			0
#define task_state_NeedSchedule	0x1
#define task_state_Idle			0x2

#endif
