#ifndef __HAL_TASK_SHCEDULE_H__
#define __HAL_TASK_SHCEDULE_H__

#include <task/structs.h>
#include <hal/cpu/desc.h>
#include <hal/cpu/api.h>

__always_inline__ void hal_task_sche_yield() {
	hal_cpu_sendIntr_self(hal_cpu_intr_Schedule);
}

__always_inline__ void hal_task_sche_updOtherThdsState() {
    hal_cpu_sendIntr_allExcluSelf(hal_cpu_intr_Schedule);
}

// implemented in entry.S
extern void hal_task_sche_switch(task_Thread *from, task_Thread *to);

void hal_task_sche_further(task_Thread *prev, task_Thread *next);

int hal_task_sche_dispatch(task_Thread *tsk);

#endif