#ifndef __TASK_API_H__
#define __TASK_API_H__

#include <hal/task/api.h>
#include <cpu/api.h>

#ifdef HAL_TASK_GETLEVEL
#define task_getLevel hal_task_getLevel
#else
#error No definition of task_getLevel() for this arch
#endif
#define task_cur cpu_getvar(task_curTsk)

#define task_union(taskStruct) ((task_Union *)(taskStruct))

task_TaskStruct *task_newSubTask(void *entryAddr, u64 arg, u64 attr);

task_TaskStruct *task_newTask(void *entryAddr, u64 arg, u64 attr);

void task_sche_enable();
void task_sche_disable();
int task_sche_getState();

__always_inline__ void task_sche_updCurState() {
    register u64 tmp = task_sche_cfsTbl[task_cur->priority];
    task_cur->vRuntime += tmp;
    task_cur->state |= task_state_NeedSchedule;
}

void task_sche_updState();

void task_sche_init();

// get idle task of the specific cpu
__always_inline__ task_TaskStruct *task_idleTask(int cpuIdx) { return hal_task_idleTask(cpuIdx); }

__always_inline__ void task_sche_syncVRuntime(task_TaskStruct *task) { task->vRuntime = task_idleTask(task->cpuId)->vRuntime; }

// release cpu immediately
void task_sche_yield();
// set task_cur to sleep
void task_sche_sleep();
// task_cur sleep for **at least** x millseconds
void task_sche_msleep(int msec);
// weak specific task immediately
void task_sche_wake(task_TaskStruct *task);

void task_sche_waitReq();

void task_sche_finishReq(task_TaskStruct *tsk);

void task_sche_preempt(task_TaskStruct *task);

__always_inline__ void task_sche_mskCpu(int cpu) { Atomic_inc(cpu_cpuPtr(cpu, task_scheMsk)); }
__always_inline__ void task_sche_unmskCpu(int cpu) { Atomic_dec(cpu_cpuPtr(cpu, task_scheMsk)); }

__always_inline__ void task_sche_msk() { Atomic_inc(cpu_ptr(task_scheMsk)); }

__always_inline__ void task_sche_unmsk() { Atomic_dec(cpu_ptr(task_scheMsk)); }

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