#include <task/syscall.h>
#include <task/api.h>
#include <screen/screen.h>

void *task_syscall_tbl[task_syscall_tblSize];

void task_syscall_initTbl() {
	task_syscall_tbl[task_syscall_exit] = task_exit;
	task_syscall_tbl[task_syscall_yield] = task_sche_yield;
	task_syscall_tbl[task_syscall_waitReq] = task_sche_waitReq;
	task_syscall_tbl[task_syscall_finishReq] = task_sche_finishReq;
}

int task_syscall_init() {
	if (hal_task_syscall_init() == res_FAIL) return res_FAIL;
	// printk(screen_log, "syscall: cpu #%d: initialized\n", task_current->cpuId);
	return res_SUCC;
}

int task_syscall_toUsr(void (*entry)(u64), u64 param, void *stkPtr) {
	return hal_task_syscall_toUsr(entry, param, stkPtr);
}