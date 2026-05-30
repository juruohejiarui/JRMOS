#ifndef __TASK_MGR_H__
#define __TASK_MGR_H__

#include <task/structs.h>
#include <hal/task/api.h>

extern task_Process *task_rootProc;

cpu_declarevar(task_Thread *, task_idleThd);
cpu_declarevar(task_Thread *, task_curThd);

#define task_cur hal_task_current()

#define task_union(task) ((task_Union *)(task))

task_Thread *task_newThd(void *entryAddr, void *arg, u64 attr, task_Process *proc);

task_Process *task_newProc(u64 attr);

int task_freeProc(task_Process *proc);

int task_freeThd(task_Thread *thd);

// exit this thread with the specific return
void task_exit(i64 res);

void task_init();

void task_initIdleThd();

void task_log();

#endif