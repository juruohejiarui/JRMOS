#include <task/api.h>
#include <task/request.h>
#include <mm/mm.h>
#include <mm/mgr.h>
#include <mm/buddy.h>
#include <interrupt/api.h>
#include <screen/screen.h>
#include <cpu/api.h>
#include <lib/bit.h>
#include <lib/algorithm.h>
#include <fs/vfs/api.h>

// this variable will be used by hal/interrupt/entry.S
int task_sche_state;
Atomic task_pidCnt;

SafeList task_freeTsks, task_sleepTsks;
cpu_definevar(RBTree, task_tsks);
cpu_definevar(ListNode, task_preemptTsks);
cpu_definevar(SpinLock, task_scheLck);
cpu_definevar(Atomic, task_scheMsk);
cpu_definevar(task_TaskStruct *, task_curTsk);
cpu_definevar(u64, task_switchCnt);

u64 task_sche_cfsTbl[0x20] = {
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

void task_sche_init() {
    for (int i = 0; i < cpu_num; i++) {
        RBTree_init(cpu_cpuPtr(i, task_tsks), task_sche_cfsTreeIns, NULL);
        List_init(cpu_cpuPtr(i, task_preemptTsks));
        SpinLock_init(cpu_cpuPtr(i, task_scheLck));
        cpu_cpuPtr(i, task_scheMsk)->value = 0;
    }
    SafeList_init(&task_sleepTsks);
    SafeList_init(&task_freeTsks);
    task_sche_state = 0;
    task_pidCnt.value = 0;
}

__optimize__ void task_sche_yield() {
    register task_TaskStruct *cur = task_cur;
    cur->vRuntime += task_sche_cfsTbl[cur->priority];
    cur->state |= task_state_NeedSchedule;
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
        printk(screen_warn, "task #%d preempted by #%d\n", task_cur->pid, nxtTsk->pid);
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
    // printk(screen_log, "%d:%p:%p->%p:%p\n", 
    //     cpu_getvar(cpu_id), 
    //     task_cur, task_cur->hal.rip,
    //     nxtTsk, nxtTsk->hal.rip);
    (*cpu_ptr(task_switchCnt))++;
    hal_task_sche_switch(task_cur, nxtTsk);
    return ;

    noNeedToSche:
    SpinLock_unlock(cpu_ptr(task_scheLck));
    (*cpu_ptr(task_switchCnt))++;
    // printk(screen_log, "%d:no nxtTsk\n", cpu_getvar(cpu_id));
    return ;
}

task_ThreadStruct *task_newThread(u64 attr) {
    task_ThreadStruct *thread = mm_kmalloc(sizeof(task_ThreadStruct), mm_Attr_Shared, NULL);
    if (thread == NULL) {
        printk(screen_err, "task: failed to allocate thread structure.\n");
        return NULL;
    }
    memset(thread, 0, sizeof(task_ThreadStruct));

    mm_mgr_init(&thread->mem, attr);

    memset(thread->sigHandler, 0, sizeof(thread->sigHandler));

    SafeList_init(&thread->tskList);

    SafeList_init(&thread->fileLst);
    SafeList_init(&thread->dirLst);

    hal_task_newThread(thread, attr);

    return thread;
}

extern int task_freeThread(task_ThreadStruct *thread);
extern int task_freeTask(task_TaskStruct *task);

void task_insSubTask(task_TaskStruct *subTsk, task_ThreadStruct *thread) {
    SafeList_insTail(&thread->tskList, &subTsk->threadNd);
}

int task_delSubTask(task_TaskStruct *subTsk) {
    task_ThreadStruct *thread = subTsk->thread;
    SafeList_del(&thread->tskList, &subTsk->threadNd);
    if (SafeList_isEmpty(&thread->tskList))
        return task_freeThread(thread);
    return res_SUCC;
}

int task_freeThread(task_ThreadStruct *thread) {
    int res;
    // close files and dirs
    mm_mgr_free(&thread->mem);
    if ((res = hal_task_freeThread(thread)) != res_SUCC) return res | res_DAMAGED;
    mm_kfree(thread, mm_Attr_Shared);
    return res_SUCC;
}

int task_freeTask(task_TaskStruct *tsk) {
    // printk(screen_log, "  ->%d\n", intr_state());
    if (hal_task_freeTask(tsk) == res_FAIL) return res_FAIL;
    
    if (task_delSubTask(tsk) == res_FAIL) {
        printk(screen_err, "task: failed to delete subtask #%ld from thread.\n", tsk->pid);
        return res_FAIL;
    }
    return mm_kfree(tsk, mm_Attr_Shared);
}

void task_initIdle() {
    task_cur->pid = task_pidCnt.value;
    Atomic_inc(&task_pidCnt);
    task_cur->priority = 0;
    task_cur->state = task_state_Running;
    task_cur->flag = 0;

    task_cur->vRuntime = 0;

    memset(&task_cur->signal, 0, sizeof(task_cur->signal));

    task_cur->thread = task_newThread(task_attr_Builtin);
    
    task_insSubTask(task_cur, task_cur->thread);

    List_init(&task_cur->scheNd);

    hal_task_initIdle();

    printk(screen_log, "task: initialize idle task of cpu %d\n", cpu_getvar(cpu_id));
}

// insert new task to cfs tree
__always_inline__ void _insNewTsk(task_TaskStruct *tsk) {
    task_sche_syncVRuntime(tsk);

    SpinLock_lockMask(cpu_cpuPtr(tsk->cpuId, task_scheLck));
    RBTree_ins(cpu_cpuPtr(tsk->cpuId, task_tsks), &tsk->rbNd);
    RBTree_debug(cpu_cpuPtr(tsk->cpuId, task_tsks));
    SpinLock_unlockMask(cpu_cpuPtr(tsk->cpuId, task_scheLck));
}

task_TaskStruct *_newTask(void *entryAddr, u64 arg, u64 attr, task_ThreadStruct *thread) {
    task_Union *tskUnion = mm_kmalloc(sizeof(task_Union), mm_Attr_Shared, NULL);
    if (tskUnion == NULL) {
        printk(screen_err, "task: failed to allocate task structure.\n");
        return NULL;
    }
    memset(tskUnion, 0, sizeof(task_Union));
    task_TaskStruct *tsk = &tskUnion->task;

    tsk->pid = task_pidCnt.value;
    Atomic_inc(&task_pidCnt);
    tsk->state = task_state_Idle;

    List_init(&task_cur->scheNd);

    tsk->thread = thread;
    
    hal_task_newTask(tsk, entryAddr, arg, attr);

    task_insSubTask(tsk, tsk->thread);

    if (hal_task_dispatchTask(tsk) == res_FAIL) {
        printk(screen_err, "task: failed to dispatch task #%ld.\n", tsk->pid);
        task_delSubTask(tsk);
        mm_kfree(tskUnion, mm_Attr_Shared);
        return NULL;
    }

    _insNewTsk(tsk);


    return tsk;
}

task_TaskStruct *task_newSubTask(void *entryAddr, u64 arg, u64 attr) {
    return _newTask(entryAddr, arg, attr, task_cur->thread);
}

task_TaskStruct *task_newTask(void *entryAddr, u64 arg, u64 attr) {
    task_ThreadStruct *thread = task_newThread(attr);
    if (thread == NULL) {
        printk(screen_err, "task: failed to crt thread for new task.\n");
        return NULL;
    }
    task_TaskStruct *tsk = _newTask(entryAddr, arg, attr, thread);
    return tsk;
}

void task_exit(u64 res) {
    /// @todo free simd struct
    hal_task_exit(res);
    task_cur->priority = task_Priority_Lowest;
    task_cur->state = task_state_NeedFree;
    
    while (1) task_sche_yield();
}

task_TaskStruct *task_sche_freeMgrTsk;

u64 task_freeMgr(u64 arg) {
    u64 tot = 0;
    while (1) {
        while (SafeList_isEmpty(&task_freeTsks)) {
            task_cur->priority = task_Priority_Lowest;
            task_sche_yield();
        }
        task_cur->priority = task_Priority_Running;
        while (!SafeList_isEmpty(&task_freeTsks)) {
            task_TaskStruct *tsk = container(SafeList_getHead(&task_freeTsks), task_TaskStruct, scheNd);
            SafeList_del(&task_freeTsks, &tsk->scheNd);
            u64 pid = tsk->pid;
            if (task_freeTask(tsk) == res_FAIL) {
                printk(screen_err, "task: failed to free task #%ld.\n", pid);
            } else printk(screen_succ, "task: task #%ld is killed, tot=%ld\n", pid, ++tot);
        }
    }
    return 0;
}