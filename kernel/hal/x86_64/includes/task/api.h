#ifndef __HAL_TASK_MGR_H__
#define __HAL_TASK_MGR_H__

#include <task/structs.h>
#include <hal/cpu/desc.h>
#include <cpu/api.h>

#define HAL_TASK_GETLEVEL

__always_inline__ int hal_task_getLevel() {
    int cs;
    __asm__ volatile (
        "movq %%cs, %%rax   \n\t"
        : "=a"(cs)
        :
        : "memory"
    );
    return cs & 3 ? task_level_User : task_level_Kernel;
}

__always_inline__ task_Thread *hal_task_current() {
	struct task_Thread * current = NULL;
	__asm__ volatile ("andq %%rsp,%0	\n\t":
        "=r"(current):
        "0"(~(task_krlStkSize - 1)));
	return current;
}

void hal_task_newThd(task_Thread *tsk, void *entryAddr, void *arg, u64 attr);

void hal_task_newProc(task_Process *proc, u64 attr);

int hal_task_freeThread(task_Process *proc);

int hal_task_freeTask(task_Thread *task);

void hal_task_exit(u64 res);

// get the idle task of the specific cpu
__always_inline__ task_Thread *hal_task_idleThread(int cpuIdx) {
    return &((task_Union *)cpu_getCpuVar(cpuIdx, hal_cpu_initStk))->thd;
}

void hal_task_initIdleThd();

#endif