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


task_TaskStruct *task_newSubTask(void *entryAddr, u64 arg, u64 attr);

void task_sche_enable();
void task_sche_disable();
int task_sche_getState();

void task_sche_updCurState();

void task_sche_updState();

void task_sche_init();

void task_sche_release();

void task_schedule();

void task_initIdle();

void task_exit(u64 res);

u64 task_freeMgr(u64 arg);

#endif