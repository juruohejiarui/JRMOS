#ifndef __TASK_CONSTANT_H__
#define __TASK_CONSTANT_H__

#include <lib/dtypes.h>

#define task_krlStkSize	(32768ul)
// 8 MB
#define task_usrStkSize	(1ull << 23)

#define task_krlAddrSt	(0xffff800000000000ul)

#define task_level_Kernel   0x0
#define task_level_User     0x3

#define task_attr_Usr       0x1ul
#define task_attr_Builtin   0x2ul

/*
states of task
bit 0 indicates whether the task need to be scheduled
*/

#define task_state_Running      0x0
#define task_state_NeedSchedule 0x1
#define task_state_Idle			0x2
#define task_state_NeedSleep    0x3
#define task_state_Sleep        0x4
#define task_state_NeedFree     0x5
#define task_state_Free         0x6
#define task_state_NeedPreempt  0x7


#define task_nrSignal	64

#define task_Priority_Running   0x0
#define task_Priority_Lowest    0x6

#define task_Signal_kill	0x0
#define task_Signal_Int		0x1
#endif