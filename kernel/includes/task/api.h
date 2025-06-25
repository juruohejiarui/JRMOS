#ifndef __TASK_API_H__
#define __TASK_API_H__

#include <hal/task/api.h>
#include <cpu/api.h>

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

#define task_union(taskStruct) ((task_Union *)(taskStruct))

task_TaskStruct *task_newSubTask(void *entryAddr, u64 arg, u64 attr);

task_TaskStruct *task_newTask(void *entryAddr, u64 arg, u64 attr);

void task_sche_enable();
void task_sche_disable();
int task_sche_getState();

void task_sche_updCurState();

void task_sche_updState();

void task_sche_init();

// get idle task of the specific cpu
__always_inline__ task_TaskStruct *task_idleTask(int cpuIdx) { return hal_task_idleTask(cpuIdx); }

__always_inline__ void task_sche_syncVRuntime(task_TaskStruct *task) { task->vRuntime = task_idleTask(task->cpuId)->vRuntime; }

// release cpu immediately
void task_sche_yield();

void task_sche_wake(task_TaskStruct *task);

void task_sche_waitReq();

void task_sche_finishReq(task_TaskStruct *tsk);

void task_sche_preempt(task_TaskStruct *task);

__always_inline__ void task_sche_msk() { Atomic_inc(cpu_getvar(scheMsk)); }

__always_inline__ void task_sche_unmsk() { Atomic_dec(cpu_getvar(scheMsk)); }

void task_sche();

void task_initIdle();

void task_exit(u64 res);

u64 task_freeMgr(u64 arg);

extern task_TaskStruct *task_sche_freeMgrTsk;

void task_signal_setHandler(u64 signal, void (*handler)(u64), u64 param);

// send signal to task
// this cannot be used in interrupt program
void task_signal_send(task_TaskStruct *target, u64 signal);

// send signal to task from interrupt program
// this will handled by task_signal_mgrTsk
void task_signal_sendFromIntr(task_TaskStruct *target, u64 signal);

extern task_TaskStruct *task_signal_mgrTsk;

void task_signal_scan();

void task_signal_init();

#endif