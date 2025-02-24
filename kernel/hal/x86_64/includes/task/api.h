#ifndef __HAL_TASK_API_H__
#define __HAL_TASK_API_H__

#include <task/constant.h>

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

#endif