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

u64 task_sche_cfsTbl[0x20] = {
    0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40
};

void task_sche_enable() { printk(screen_log, "enable sche\n"); task_sche_state = 1; }
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

// set task_cur to sleep
void task_sche_sleep() {
    task_cur->state = task_state_NeedSleep;
    task_sche_yield();
}

__always_inline__ void _preempt(task_TaskStruct *task) {
    switch (task->state) {
        case task_state_Idle:
            printk(screen_warn, "task #%d: preempt #%d from idle\n", task_cur->pid, task->pid);
            RBTree_del(cpu_cpuPtr(task->cpuId, task_tsks), &task->rbNd);
            List_insTail(cpu_cpuPtr(task->cpuId, task_preemptTsks), &task->scheNd);
            task->state = task_state_NeedPreempt;
            break;
       
        case task_state_Sleep:
            printk(screen_warn, "task #%d: preempt #%d from sleep\n", task_cur->pid, task->pid);
            SafeList_del(&task_sleepTsks, &task->scheNd);
            List_insTail(cpu_cpuPtr(task->cpuId, task_preemptTsks), &task->scheNd);
            task->state = task_state_NeedPreempt;
            break;
        case task_state_NeedSleep:
        case task_state_NeedSchedule:
            printk(screen_warn, "task #%d: preempt #%d from need schedule/need sleep\n", task_cur->pid, task->pid);
            task->state = task_state_Running;
            break;
    }
}

__optimize__ void task_sche_preempt(task_TaskStruct *task) {
    // mask interrupt
    {
        register u32 state = task->state;
        if (state == task_state_Running || state == task_state_NeedPreempt) return ;
    }
    task_sche_mskCpu(task->cpuId);
    SpinLock_lockMask(cpu_cpuPtr(task->cpuId, task_scheLck));
    _preempt(task);
    SpinLock_unlockMask(cpu_cpuPtr(task->cpuId, task_scheLck));
    task_sche_unmsk(task->cpuId);
}

void task_sche_waitReq() {
	Atomic_inc(&task_cur->reqWait);
	// try to yield, scheduler will put current task to sleep task list
	while (task_cur->reqWait.value > 0)
        task_sche_yield();
}

void task_sche_finishReq(task_TaskStruct *task) {
    Atomic_dec(&task->reqWait);
    if (task->reqWait.value == 0) {
        SpinLock_lockMask(cpu_cpuPtr(task->cpuId, task_scheLck));
        // if the task is in sleep state, move it back to running state
        if (task->state == task_state_Sleep) {
            SafeList_del(&task_sleepTsks, &task->scheNd);
            task_sche_syncVRuntime(task);
            RBTree_ins(cpu_cpuPtr(task->cpuId, task_tsks), &task->rbNd);
            task->state = task_state_Idle;
            // sync the vRuntime of the task
        }
        SpinLock_unlockMask(cpu_cpuPtr(task->cpuId, task_scheLck));
    }
}

// state transition for current task when switch to other task
__always_inline__ void task_sche_hangCur() {
    // wait for request, then switch to idle task
    register task_TaskStruct *cur = task_cur;
    if (cur->reqWait.value > 0)
        cur->state = task_state_Sleep;
    switch (cur->state) {
        case task_state_NeedFree :
            cur->state = task_state_Free;
            SafeList_insTail(&task_freeTsks, &cur->scheNd);
            break;
        case task_state_NeedSleep :
            cur->state = task_state_Sleep;
            SafeList_insTail(&task_sleepTsks, &cur->scheNd);
            printk(screen_warn, "task: #%d sleep.\n", cur->pid);
            break;
        default:
            cur->state = task_state_Idle;
            RBTree_ins(cpu_ptr(task_tsks), &cur->rbNd);
            break;
    }
}

// main process of scheduling, take the next task and switch to it
__optimize__ void task_sche() {
    SpinLock_lock(cpu_ptr(task_scheLck));
    task_TaskStruct *nxtTsk;
    RBTree *tsks;
    if (__unlikely__(cpu_getvar(cpu_id) == 0) && __unlikely__(!SafeList_isEmpty(&task_freeTsks)))
        _preempt(task_sche_freeMgrTsk);
    // search for the next task
    if (!List_isEmpty(cpu_ptr(task_preemptTsks))) {
        nxtTsk = container(List_getHead(cpu_ptr(task_preemptTsks)), task_TaskStruct, scheNd);
        printk(screen_log, "task #%d preempted by #%d\n", task_cur->pid, nxtTsk->pid);
        List_del(&nxtTsk->scheNd);
    } else {
        RBNode *nxtTskNd = RBTree_getLeft(tsks = cpu_ptr(task_tsks));
        if (!nxtTskNd) goto noNeedToSche;
        nxtTsk = container(nxtTskNd, task_TaskStruct, rbNd);
        if (task_cur->state != task_state_NeedSleep && nxtTsk->vRuntime > task_cur->vRuntime) 
            goto noNeedToSche;
        RBTree_del(tsks, &nxtTsk->rbNd);
    }
    task_sche_hangCur();
    nxtTsk->state = task_state_Running;
    SpinLock_unlock(cpu_ptr(task_scheLck));
    hal_task_sche_switch(task_cur, nxtTsk);
    return ;

    noNeedToSche:
    SpinLock_unlock(cpu_ptr(task_scheLck));
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
        if (__likely__(SafeList_isEmpty(&task_freeTsks))) task_sche_sleep();
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