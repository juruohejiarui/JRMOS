#include "inner.h"
#include <screen/screen.h>
#include <cpu/api.h>
#include <lib/bit.h>
#include <lib/algorithm.h>

// this variable will be used by hal/interrupt/entry.S
int task_sche_state;

cpu_definevar(RBTree, task_tsks);
cpu_definevar(ListNode, task_preemptTsks);
cpu_definevar(SpinLock, task_scheLck);
cpu_definevar(Atomic, task_scheMsk);
cpu_definevar(task_TaskStruct *, task_curTsk);
cpu_definevar(u64, task_switchCnt);

const u64 task_sche_cfsTbl[0x20] = {
    0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40
};

void task_sche_enable() { task_sche_state = 1; }
void task_sche_disable() { task_sche_state = 0; }
int task_sche_getState() { return task_sche_state; }

__always_inline__ int task_sche_cfsTreeCmp(RBNode *a, RBNode *b) {
    register task_TaskStruct
        *ta = container(a, task_TaskStruct, rbNd),
        *tb = container(b, task_TaskStruct, rbNd);
    return (ta->vRuntime != tb->vRuntime ? ta->vRuntime < tb->vRuntime : (u64)ta < (u64)tb);
}

RBTree_insertDef(task_sche_cfsTreeIns, task_sche_cfsTreeCmp)

__always_inline__ void _updOtherState() {
    hal_task_sche_updOtherState();
}

__optimize__ void task_sche_updState() {
    if (__unlikely__(!task_sche_state)) return ;
    task_sche_updCurState();
    _updOtherState();
}

__optimize__ void task_sche_yield() {
    task_sche_updCurState();
    hal_task_sche_yield();
}

void task_sche_wake(task_TaskStruct *task) {
    SpinLock_lockMask(cpu_cpuPtr(task->cpuId, task_scheLck));
    if (task->state == task_state_Sleep) {
        SafeList_del(&task_sleepTsks, &task->scheNd);
        RBTree_ins(cpu_cpuPtr(task->cpuId, task_tsks), &task->rbNd);
        task->state = task_state_Idle;
    }
    SpinLock_unlockMask(cpu_cpuPtr(task->cpuId, task_scheLck));
}

__always_inline__ void _preempt(task_TaskStruct *task) {
    // mask interrupt
    {
        register u32 state = task->state;
        if (state == task_state_Running || state == task_state_NeedPreempt) return ;
    }
    switch (task->state) {
        case task_state_Idle:
            RBTree_del(cpu_cpuPtr(task->cpuId, task_tsks), &task->rbNd);
            List_insTail(cpu_cpuPtr(task->cpuId, task_preemptTsks), &task->scheNd);
            task->state = task_state_NeedPreempt;
            break;
        case task_state_NeedSchedule:
            task->state = task_state_Running;
            break;
        case task_state_Sleep:
            SafeList_del(&task_sleepTsks, &task->scheNd);
            List_insTail(cpu_cpuPtr(task->cpuId, task_preemptTsks), &task->scheNd);
            task->state = task_state_NeedPreempt;
            break;
        default:
            break;
    }
    // reflesh vruntime using task_cur
    task->vRuntime = task_cur->vRuntime;
}

__optimize__ void task_sche_preempt(task_TaskStruct *task) {
    SpinLock_lockMask(cpu_cpuPtr(task->cpuId, task_scheLck));
    _preempt(task);
    SpinLock_unlockMask(cpu_cpuPtr(task->cpuId, task_scheLck));
}

void task_sche_waitReq() {
	Atomic_inc(&task_cur->reqWait);
	// try to yield, scheduler will put current task to sleep task list
	while (task_cur->reqWait.value > 0)
        task_sche_yield();
}

void task_sche_finishReq(task_TaskStruct *task) {
    SpinLock_lockMask(cpu_cpuPtr(task->cpuId, task_scheLck));
    Atomic_dec(&task->reqWait);
    if (task->reqWait.value == 0)
        // if the task is in sleep state, move it back to running state
        _preempt(task);
    SpinLock_unlockMask(cpu_cpuPtr(task->cpuId, task_scheLck));
}

// state transition for current task when switch to other task
__always_inline__ void task_sche_hangCur() {
    // wait for request, then switch to idle task
    if (task_cur->reqWait.value > 0) {
        SafeList_insTail(&task_sleepTsks, &task_cur->scheNd);
        task_cur->state = task_state_Sleep;
        printk(screen_warn, "task: #%d sleep for %d requests.\n", task_cur->pid, task_cur->reqWait.value);
        return ;
    }
    switch (task_cur->state) {
        case task_state_NeedFree :
            task_cur->state = task_state_Free;
            SafeList_insTail(&task_freeTsks, &task_cur->scheNd);
            break;
        default:
            task_cur->state = task_state_Idle;
            RBTree_ins(cpu_ptr(task_tsks), &task_cur->rbNd);
            break;
    }
}

// main process of scheduling, take the next task and switch to it
__optimize__ void task_sche() {
    SpinLock_lock(cpu_ptr(task_scheLck));
    task_TaskStruct *nxtTsk;
    RBTree *tsks;
    // search for the next task
    if (!List_isEmpty(cpu_ptr(task_preemptTsks))) {
        nxtTsk = container(List_getHead(cpu_ptr(task_preemptTsks)), task_TaskStruct, scheNd);
        List_del(&nxtTsk->scheNd);
    } else {
        RBNode *nxtTskNd = RBTree_getLeft(tsks = cpu_ptr(task_tsks));
        if (!nxtTskNd) goto noNeedToSche;
        nxtTsk = container(nxtTskNd, task_TaskStruct, rbNd);
        if (nxtTsk->vRuntime > task_cur->vRuntime) goto noNeedToSche;
        RBTree_del(tsks, &nxtTsk->rbNd);
    }
    task_sche_hangCur();
    nxtTsk->state = task_state_Running;
    SpinLock_unlock(cpu_ptr(task_scheLck));
    (*cpu_ptr(task_switchCnt))++;
    hal_task_sche_switch(task_cur, nxtTsk);
    return ;

    noNeedToSche:
    SpinLock_unlock(cpu_ptr(task_scheLck));
    return ;
}

void task_sche_init() {
    for (int i = 0; i < cpu_num; i++) {
        RBTree_init(cpu_cpuPtr(i, task_tsks), task_sche_cfsTreeIns, NULL);
        List_init(cpu_cpuPtr(i, task_preemptTsks));
        SpinLock_init(cpu_cpuPtr(i, task_scheLck));
        cpu_cpuPtr(i, task_scheMsk)->value = 0;
    }
    SafeList_init(&task_sleepTsks);
    SafeList_init(&task_freeTsks);
}