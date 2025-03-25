#ifndef __TASK_CONSTANT_H__
#define __TASK_CONSTANT_H__

#include <lib/dtypes.h>

#define task_krlStkSize	(32768ul)
// 8 MB
#define task_usrStkSize	(1ull << 23)

#define task_krlAddrSt	(0xffff800000000000ul)
#define task_usrStkTop  (0x0000fffffffffff0ul)

#define task_level_Kernel   0x0
#define task_level_User     0x3

#define task_attr_Usr       0x1ul
#define task_attr_Builtin   0x2ul

#define task_state_Running      0x0
#define task_state_NeedSchedule 0x1
#define task_state_Idle         0x2

#define task_flag_WaitFree     0x1ul

#define task_nrSignal	64

#endif