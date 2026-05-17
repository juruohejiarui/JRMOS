#ifndef __TASK_INNER_H__
#define __TASK_INNER_H__

#include <task/api.h>
#include <task/schedule.h>
#include <task/request.h>
#include <task/signal.h>

extern Atomic task_pidCnt, task_threadCnt;

extern SafeList task_freeThds, task_sleepThds;

task_Thread *_freeMgrThd, _signalMgrThd;

typedef struct WaitReq {
    task_Thread *thd;
    u64 targetState;
    ListNode lstNd;
} WaitReq;

cpu_declarevar(SafeList, task_sche_reqs);

void _sche_init();

void _signal_init();

#endif