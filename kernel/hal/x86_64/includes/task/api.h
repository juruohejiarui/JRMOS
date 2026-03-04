#ifndef __HAL_TASK_API_H__
#define __HAL_TASK_API_H__

#include <task/constant.h>
#include <task/structs.h>
#include <cpu/api.h>
#include <hal/cpu/desc.h>

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


// get the idle task of the specific cpu
__always_inline__ task_TaskStruct *hal_task_idleTask(int cpuIdx) {
    return &((task_Union *)cpu_getCpuVar(cpuIdx, hal_cpu_initStk))->task;
}

int hal_task_dispatchTask(task_TaskStruct *tsk);

void hal_task_sche_yield();

void hal_task_sche_switch(task_TaskStruct *from, task_TaskStruct *to);

void hal_task_sche_switchTss(task_TaskStruct *prev, task_TaskStruct *next);

void hal_task_sche_updOtherState();

void hal_task_newThread(task_ThreadStruct *thread, u64 attr);

void hal_task_initIdle();

void hal_task_newSubTask(task_TaskStruct *tsk, void *entryAddr, u64 arg, u64 attr);

void hal_task_newTask(task_TaskStruct *tsk, void *entryAddr, u64 arg, u64 attr);

int hal_task_freeThread(task_ThreadStruct *thread);

int hal_task_freeTask(task_TaskStruct *task);

void hal_task_exit(u64 res);

#endif