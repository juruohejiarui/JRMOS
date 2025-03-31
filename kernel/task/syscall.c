#include <task/syscall.h>
#include <task/api.h>
#include <screen/screen.h>

void *task_syscall_tbl[task_syscall_tblSize];

void task_syscall_initTbl() {
	task_syscall_tbl[task_syscall_exit] = task_exit;
	task_syscall_tbl[task_syscall_release] = task_sche_yield;
}

int task_syscall_init() {
	if (hal_task_syscall_init() == res_FAIL) return res_FAIL;
	printk(WHITE, BLACK, "syscall: cpu #%d: initialized\n", task_current->cpuId);
	return res_SUCC;
}

int task_syscall_toUsr(void (*entry)(u64), u64 param, void *stkPtr) {
	return hal_task_syscall_toUsr(entry, param, stkPtr);
}