#include <hal/task/api.h>

void hal_task_sche_switchTss(task_TaskStruct *prev, task_TaskStruct *next) {
    
	__asm__ volatile ( "movq %%fs, %0" : "=a"(prev->hal.fs) : : "memory");
	__asm__ volatile ( "movq %%gs, %0" : "=a"(prev->hal.gs) : : "memory");

    __asm__ volatile ( "movq %0, %%fs" : "=a"(next->hal.fs) : : "memory");
	__asm__ volatile ( "movq %0, %%gs" : "=a"(next->hal.gs) : : "memory");
}

void hal_task_sche_updOtherState() {
    
}