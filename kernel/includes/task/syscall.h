#ifndef __TASK_SYSCALL_H__
#define __TASK_SYSCALL_H__

#include <lib/dtypes.h>
#include <hal/task/syscall.h>

#define task_syscall_tblSize 0x100
extern void *task_syscall_tbl[task_syscall_tblSize];

int task_syscall_init();

#define task_syscall0 hal_task_syscall0
#define task_syscall1 hal_task_syscall1
#define task_syscall2 hal_task_syscall2
#define task_syscall3 hal_task_syscall3
#define task_syscall4 hal_task_syscall4
#define task_syscall5 hal_task_syscall5
#define task_syscall6 hal_task_syscall6

#endif