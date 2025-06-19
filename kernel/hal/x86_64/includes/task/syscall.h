#ifndef __HAL_TASK_SYSCALL_H__
#define __HAL_TASK_SYSCALL_H__

#include <lib/dtypes.h>
#include <lib/funcgen.h>
#include <hal/interrupt/desc.h>

#define _call_syscallInst \
__asm__ volatile ( \
	"syscall			\n\t" \
	: "=a"(val) \
	: "D"(idx), "S"((u64)&regs) \
	: "memory" \
); 

__always_inline__ u64 hal_task_syscall_api0(u64 idx) {
	hal_intr_PtReg regs; u64 val;
	_call_syscallInst
	return val;
}

__always_inline__ u64 hal_task_syscall_api1(u64 idx, u64 arg0) {
	hal_intr_PtReg regs; u64 val;
	regs.rdi = arg0;
	_call_syscallInst
	return val;
}

__always_inline__ u64 hal_task_syscall_api2(u64 idx, u64 arg0, u64 arg1) {
	hal_intr_PtReg regs; u64 val;
	regs.rdi = arg0;
	regs.rsi = arg1;
	_call_syscallInst
	return val;
}

__always_inline__ u64 hal_task_syscall_api3(u64 idx, u64 arg0, u64 arg1, u64 arg2) {
	hal_intr_PtReg regs; u64 val;
	regs.rdi = arg0;
	regs.rsi = arg1;
	regs.rdx = arg2;
	_call_syscallInst
	return val;
}

__always_inline__ u64 hal_task_syscall_api4(u64 idx, u64 arg0, u64 arg1, u64 arg2, u64 arg3) {
	hal_intr_PtReg regs; u64 val;
	regs.rdi = arg0;
	regs.rsi = arg1;
	regs.rdx = arg2;
	regs.rcx = arg3;
	_call_syscallInst
	return val;
}

__always_inline__ u64 hal_task_syscall_api5(u64 idx, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4) {
	hal_intr_PtReg regs; u64 val;
	regs.rdi = arg0;
	regs.rsi = arg1;
	regs.rdx = arg2;
	regs.rcx = arg3;
	regs.r8 = arg4;
	_call_syscallInst
	return val;
}

__always_inline__ u64 hal_task_syscall_api6(u64 idx, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
	hal_intr_PtReg regs; u64 val;
	regs.rdi = arg0;
	regs.rsi = arg1;
	regs.rdx = arg2;
	regs.rcx = arg3;
	regs.r8 = arg4;
	regs.r9 = arg5;
	_call_syscallInst

	return val;
}

#undef _call_syscallInst

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

int hal_task_syscall_toUsr(void (*entry)(u64), u64 param, void *stkPtr);

#endif