#ifndef __TASK_MGR_H__
#define __TASK_MGR_H__

#include <task/structs.h>
#include <hal/task/api.h>

extern task_Process *task_rootProc;

cpu_declarevar(task_Thread *, task_idleTsk);

#define task_cur hal_task_current()

#define task_union(task) ((task_Union *)(task))

task_Thread *task_newThd(void *entryAddr, void *arg, u64 attr, task_Process *proc);

task_Process *task_newProc(u64 attr);

void task_exit(i64 res);

void task_init();

#endif