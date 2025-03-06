#ifndef __TASK_API_H__
#define __TASK_API_H__

#include <hal/task/api.h>

#ifdef HAL_TASK_GETLEVEL
#define task_getLevel hal_task_getLevel
#else
#error No definition of task_getLevel() for this arch
#endif

#ifdef HAL_TASK_CURRENT
#define task_current hal_task_current
#else
#error No definition of task_current() for this arch!
#endif


task_TaskStruct *task_newSubTask(void *entryAddr, void *arg);

void task_sche_updState();

void task_sche_init();

void task_sche_release();

void task_schedule();

#endif