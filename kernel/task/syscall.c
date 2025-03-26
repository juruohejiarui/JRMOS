#include <task/syscall.h>

void *task_syscall_tbl[task_syscall_tblSize];

int task_syscall_init() {
	if (!hal_task_syscall_init()) return res_FAIL;
	return res_SUCC;
}