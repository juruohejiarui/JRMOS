#ifndef __HAL_TASK_API_H__
#define __HAL_TASK_API_H__

#include <task/constant.h>
#include <task/structs.h>

#define HAL_TASK_GETLEVEL

static __always_inline__ int hal_task_getLevel() {
    int cs;
    __asm__ volatile (
        "movq %%cs, %%rax   \n\t"
        : "=a"(cs)
        :
        : "memory"
    );
    return cs ? task_level_User : task_level_Kernel;
}

#define HAL_TASK_CURRENT

static __always_inline__ task_TaskStruct *hal_task_getCurrent() {
    register task_TaskStruct *task;
	__asm__ volatile ( " andq %%rsp, %0		\n\t" : "=r"(task) : "0"(~(task_krlStkSize - 1)) : "memory");
    return task;
}

#define hal_task_current hal_task_getCurrent()

#endif