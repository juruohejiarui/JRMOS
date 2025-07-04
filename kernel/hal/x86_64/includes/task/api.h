#ifndef __HAL_TASK_API_H__
#define __HAL_TASK_API_H__

#include <task/constant.h>
#include <task/structs.h>

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

#define HAL_TASK_CURRENT

__always_inline__ task_TaskStruct *hal_task_getCurrent() {
    register task_TaskStruct *task;
	__asm__ volatile ( " andq %%rsp, %0		\n\t" : "=r"(task) : "0"(~(task_krlStkSize - 1)) : "memory");
    return task;
}

#define hal_task_current hal_task_getCurrent()

// get the idle task of the specific cpu
__always_inline__ task_TaskStruct *hal_task_idleTask(int cpuIdx) {
    return &((task_Union *)cpu_desc[cpuIdx].hal.initStk)->task;
}

int hal_task_dispatchTask(task_TaskStruct *tsk);

void hal_task_sche_yield();

void hal_task_sche_switch(task_TaskStruct *to);

void hal_task_sche_switchTss(task_TaskStruct *prev, task_TaskStruct *next);

void hal_task_sche_updOtherState();

void hal_task_newThread(hal_task_ThreadStruct *thread, u64 attr);

void hal_task_initIdle();

void hal_task_newSubTask(task_TaskStruct *tsk, void *entryAddr, u64 arg, u64 attr);

void hal_task_newTask(task_TaskStruct *tsk, void *entryAddr, u64 arg, u64 attr);

int hal_task_freeThread(task_ThreadStruct *thread);

int hal_task_freeTask(task_TaskStruct *task);

void hal_task_exit(u64 res);

#endif