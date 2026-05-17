#ifndef __TASK_SCHEDULE_H__
#define __TASK_SCHEDULE_H__

#include <task/structs.h>
#include <cpu/api.h>

/* nice值到权重的映射表 */
extern const int task_sche_cfsWeight[40];

cpu_declarevar(u64, task_sche_switchCnt);
cpu_declarevar(Atomic, task_sche_msk);

__always_inline__ void task_sche_updCurThdState() {
    task_Thread *cur = task_cur;
    cur->vRuntime += task_sche_cfsWeight[cur->priority + 20];
    cur->state |= task_state_NeedSchedule;
}

void task_sche_updAllCurThdsState();

// release cpu immediately
void task_sche_yield();
// task_cur sleep for **at least** x millseconds
void task_sche_msleep(int msec);
// weak specific task immediately
void task_sche_wake(task_Thread *thd);
// let task preempt immediately in next time slice
void task_sche_preempt(task_Thread *thd);

void task_sche_enable();

void task_sche_disable();

int task_sche_chkEnable();

__always_inline__ void task_sche_pause() { Atomic_inc(cpu_ptr(task_sche_msk)); }

__always_inline__ void task_sche_resume()  { Atomic_dec(cpu_ptr(task_sche_msk)); }

__always_inline__ void task_sche_pauseCpu(int id) { Atomic_inc(cpu_cpuPtr(id, task_sche_msk)); }

__always_inline__ void task_sche_resumeCpu(int id) { Atomic_dec(cpu_cpuPtr(id, task_sche_msk)); }

void task_sche_trySche();

#endif