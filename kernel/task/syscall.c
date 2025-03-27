#include <task/syscall.h>
#include <task/api.h>

void *task_syscall_tbl[task_syscall_tblSize];

int task_syscall_init() {
	if (!hal_task_syscall_init()) return res_FAIL;

	task_syscall_tbl[task_syscall_exit] = task_exit;
	return res_SUCC;
}

int task_syscall_toUsr(void (*entry)(u64), u64 param, void *stkPtr) {
	return hal_task_syscall_toUsr(entry, param, stkPtr);
}