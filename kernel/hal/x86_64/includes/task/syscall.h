#ifndef __HAL_TASK_SYSCALL_H__
#define __HAL_TASK_SYSCALL_H__

#include <lib/dtypes.h>
#include <lib/funcgen.h>
#include <hal/interrupt/desc.h>

u64 hal_task_syscall_api0(u64 index);
u64 hal_task_syscall_api1(u64 index, u64 arg0);
u64 hal_task_syscall_api2(u64 index, u64 arg0, u64 arg1);
u64 hal_task_syscall_api3(u64 index, u64 arg0, u64 arg1, u64 arg2);
u64 hal_task_syscall_api4(u64 index, u64 arg0, u64 arg1, u64 arg2, u64 arg3);
u64 hal_task_syscall_api5(u64 index, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4);
u64 hal_task_syscall_api6(u64 index, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5);

#define hal_task_syscall0(index) \
	hal_task_syscall_api0(index)
#define hal_task_syscall(x, index, ...) \
	hal_task_syscall_api##x(index, map(x, (u64), __VA_ARGS__))

#define hal_task_syscall1(index, ...) \
	hal_task_syscall(1, index, __VA_ARGS__)

#define hal_task_syscall2(index, ...) \
	hal_task_syscall(2, index, __VA_ARGS__)

#define hal_task_syscall3(index, ...) \
	hal_task_syscall(3, index, __VA_ARGS__)

#define hal_task_syscall4(index, ...) \
	hal_task_syscall(4, index, __VA_ARGS__)

#define hal_task_syscall5(index, ...) \
	hal_task_syscall(5, index, __VA_ARGS__)

#define hal_task_syscall6(index, ...) \
	hal_task_syscall(6, index, __VA_ARGS__)

void hal_task_syscall_entryKrl();
void hal_task_syscall_backUsr();

int hal_task_syscall_init();

#endif