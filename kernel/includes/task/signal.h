#ifndef __TASK_SIGNAL_H__
#define __TASK_SIGNAL_H__

#include <task/structs.h>

void task_signal_setHandler(u64 signal, void (*handler)(i64), u64 param);

// send signal to task
// this cannot be used in interrupt program
void task_signal_send(task_Thread *target, u64 signal);

// send signal to task from interrupt program
// this will handled by task_signal_mgrTsk
void task_signal_sendFromIntr(task_Thread *target, u64 signal);

extern task_Thread *_mgrTsk;

void task_signal_scan();
#endif