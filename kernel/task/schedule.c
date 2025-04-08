#include <task/api.h>
#include <mm/mm.h>
#include <mm/buddy.h>
#include <interrupt/api.h>
#include <screen/screen.h>
#include <cpu/api.h>
#include <lib/bit.h>
#include <lib/algorithm.h>

static task_MgrStruct task_mgr;

// this variable will be used by hal/interrupt/entry.S
int task_sche_state;
Atomic task_pidCnt;

static u64 task_sche_cfsTbl[0x20] = {
    0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40
};

void task_sche_enable() { task_sche_state = 1; }
void task_sche_disable() { task_sche_state = 0; }
int task_sche_getState() { return task_sche_state; }

static __always_inline__ int task_sche_cfsTreeCmp(RBNode *a, RBNode *b) {
    register task_TaskStruct
        *ta = container(a, task_TaskStruct, rbNd),
        *tb = container(b, task_TaskStruct, rbNd);
    return (ta->vRuntime != tb->vRuntime ? ta->vRuntime < tb->vRuntime : (u64)ta < (u64)tb);
}

RBTree_insert(task_sche_cfsTreeIns, task_sche_cfsTreeCmp)

void task_sche_updCurState() {
    register u64 tmp = task_sche_cfsTbl[task_current->priority];
    task_current->vRuntime += tmp;
    task_current->state |= 1;
}

static __always_inline__ void task_sche_updOtherState() {
    hal_task_sche_updOtherState();
}

void task_sche_updState() {
    if (!task_sche_state) return ;
    task_sche_updCurState();
    task_sche_updOtherState();
}

void task_sche_init() {
    for (int i = 0; i < cpu_num; i++) {
        RBTree_init(&task_mgr.tasks[i], task_sche_cfsTreeIns, task_sche_cfsTreeCmp);
        List_init(&task_mgr.preemptTsks[i]);
        SpinLock_init(&task_mgr.scheLck[i]);
        task_mgr.scheMsk[i].value = 0;

        cpu_desc[i].tsks = &task_mgr.tasks[i];
        cpu_desc[i].preemptTsks = &task_mgr.preemptTsks[i];
        cpu_desc[i].scheLck = &task_mgr.scheLck[i];
        cpu_desc[i].scheMsk = &task_mgr.scheMsk[i];
    }
    SafeList_init(&task_mgr.sleepTsks);
    SafeList_init(&task_mgr.freeTsks);
    task_sche_state = 0;
    task_pidCnt.value = 0;
}

void task_sche_yield() {
    task_current->vRuntime += max(1, task_sche_cfsTbl[task_current->priority] >> 1);
    task_current->state |= task_state_NeedSchedule;
    hal_task_sche_yield();
}

void task_sche_sleep() {
    task_current->state = task_state_NeedSleep;
    while (task_current->state == task_state_NeedSleep) task_sche_yield();
}

void task_sche_wake(task_TaskStruct *task) {
    // mask interrupt
    SpinLock_lockMask(&task_mgr.scheLck[task->cpuId]);
    if (task->state == task_state_NeedSleep) task->state = task_state_Running;
    else if (task->state == task_state_Sleep) {
        task->state = task_state_Idle;
        SafeList_del(&task_mgr.sleepTsks, &task->scheNd);
        RBTree_ins(&task_mgr.tasks[task->cpuId], &task->rbNd);
    }
    SpinLock_unlockMask(&task_mgr.scheLck[task->cpuId]);
}

void task_sche_preempt(task_TaskStruct *task) {
    // mask interrupt
    {
        register u32 state = task->state;
        if (state == task_state_Running || state == task_state_NeedPreempt) return ;
    }
    
    SpinLock_lockMask(&task_mgr.scheLck[task->cpuId]);
    switch (task->state) {
        case task_state_Idle:
            printk(WHITE, BLACK, "task #%d: preempt #%d from idle\n", task_current->pid, task->pid);
            RBTree_del(&task_mgr.tasks[task->cpuId], &task->rbNd);
            List_insTail(&task_mgr.preemptTsks[task->cpuId], &task->scheNd);
            task->state = task_state_NeedPreempt;
            break;
        case task_state_NeedSchedule:
        case task_state_NeedSleep:
            printk(WHITE, BLACK, "task #%d: preempt #%d from need schedule/need sleep\n", task_current->pid, task->pid);
            task->state = task_state_Running;
            break;
        case task_state_Sleep:
            printk(WHITE, BLACK, "task #%d: preempt #%d from sleep\n", task_current->pid, task->pid);

            SafeList_del(&task_mgr.sleepTsks, &task->scheNd);
            List_insTail(&task_mgr.preemptTsks[task->cpuId], &task->scheNd);
            task->state = task_state_NeedPreempt;
            break;
        default:
            break;
    }
    SpinLock_unlockMask(&task_mgr.scheLck[task->cpuId]);
}

int cnt = 0;

// state transition for current task when switch to other task
static __always_inline__ void task_sche_hangCur() {
    // printk(WHITE, BLACK, "hang %#018lx flag=%#018lx\n", task_current, task_current->flag);
    switch (task_current->state) {
        case task_state_NeedFree :
            // printk(WHITE, BLACK, "move #%d to free list\n", task_current->pid);
            task_current->state = task_state_Free;
            SafeList_insTail(&task_mgr.freeTsks, &task_current->scheNd);
            break;
        case task_state_NeedSleep :
            // printk(WHITE, BLACK, "move #%d to sleep list\n", task_current->pid);
            task_current->state = task_state_Sleep;
            SafeList_insTail(&task_mgr.sleepTsks, &task_current->scheNd);
            break;
        default:
            task_current->state = task_state_Idle;
            RBTree_ins(cpu_getvar(tsks), &task_current->rbNd);
            break;
    }
}

void task_sche() {
    SpinLock_lock(cpu_getvar(scheLck));
    task_TaskStruct *nxtTsk;
    // search for the next task
    if (!List_isEmpty(cpu_getvar(preemptTsks))) {
        nxtTsk = container(List_getHead(cpu_getvar(preemptTsks)), task_TaskStruct, scheNd);
        printk(WHITE, BLACK, "task #%d preempted by #%d\n", task_current->pid, nxtTsk->pid);
        List_del(&nxtTsk->scheNd);
        goto needSche;
    } else {
        RBNode *nxtTskNd = RBTree_getLeft(cpu_getvar(tsks));
        if (!nxtTskNd) goto noNeedToSche;
        nxtTsk = container(nxtTskNd, task_TaskStruct, rbNd);
        if (nxtTsk->vRuntime > task_current->vRuntime) goto noNeedToSche;
        RBTree_del(cpu_getvar(tsks), &nxtTsk->rbNd);
        goto needSche;
    }
    noNeedToSche:
    SpinLock_unlock(cpu_getvar(scheLck));
    return ;

    needSche:
    task_sche_hangCur();
    nxtTsk->state = task_state_Running;
    SpinLock_unlock(cpu_getvar(scheLck));
    // if (task_current->cpuId == 0) 
        // printk(WHITE, BLACK, "cpu #%d: #%d->#%d\n", task_current->cpuId, task_current->pid, nxtTsk->pid);
    hal_task_sche_switch(nxtTsk);
}

task_ThreadStruct *task_newThread() {
    task_ThreadStruct *thread = mm_kmalloc(sizeof(task_ThreadStruct), mm_Attr_Shared, NULL);
    if (thread == NULL) {
        printk(RED, BLACK, "task: failed to allocate thread structure.\n");
        return NULL;
    }
    memset(thread, 0, sizeof(task_ThreadStruct));
    SpinLock_lock(&mm_map_krlTblLck);

    thread->krlTblModiJiff.value = mm_map_krlTblModiJiff.value;
    thread->allocMem.value = thread->allocVirtMem.value = 0;
    
    SafeList_init(&thread->pgRecord);

    RBTree_init(&thread->slabRecord, mm_slabRecord_insert, mm_slabRecord_comparator);

    memset(thread->sigHandler, 0, sizeof(thread->sigHandler));

    SpinLock_init(&thread->pgTblLck);

    SafeList_init(&thread->tskList);

    hal_task_newThread(&thread->hal);

    SpinLock_unlock(&mm_map_krlTblLck);

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
    for (ListNode *pageList = thread->pgRecord.head.next; pageList != &thread->pgRecord.head; ) {
        ListNode *nxt = pageList->next;
        if (mm_freePages(container(pageList, mm_Page, list)) == res_FAIL) return res_FAIL;
        pageList = nxt;
    }
    while (thread->slabRecord.root) {
        mm_SlabRecord *record = container(thread->slabRecord.root, mm_SlabRecord, rbNode);
        mm_kfree(record->ptr, mm_Attr_Shared);
    }
    if (hal_task_freeThread(thread) == res_FAIL) return res_FAIL;
    mm_kfree(thread, mm_Attr_Shared);
    return res_SUCC;
}

int task_freeTask(task_TaskStruct *tsk) {
    if (task_delSubTask(tsk) == res_FAIL) {
        printk(RED, BLACK, "task: failed to delete subtask #%ld from thread.\n", tsk->pid);
        return res_FAIL;
    }
    // printk(WHITE, BLACK, "  ->%d\n", intr_state());
    if (hal_task_freeTask(tsk) == res_FAIL) return res_FAIL;
    return mm_kfree(tsk, mm_Attr_Shared);
}

void task_initIdle() {
    task_current->pid = task_pidCnt.value;
    Atomic_inc(&task_pidCnt);
    task_current->priority = 0;
    task_current->state = task_state_Running;
    task_current->flag = 0;

    task_current->vRuntime = 0;

    memset(&task_current->signal, 0, sizeof(task_current->signal));

    task_current->thread = task_newThread();
    task_insSubTask(task_current, task_current->thread);

    List_init(&task_current->scheNd);

    hal_task_initIdle();
}

// insert new task to cfs tree
static void _insertNewTsk(task_TaskStruct *tsk) {
    SpinLock_lockMask(&task_mgr.scheLck[tsk->cpuId]);
    RBTree_ins(&task_mgr.tasks[tsk->cpuId], &tsk->rbNd);
    SpinLock_unlockMask(&task_mgr.scheLck[tsk->cpuId]);
}

task_TaskStruct *task_newSubTask(void *entryAddr, u64 arg, u64 attr) {
    task_Union *tskUnion = mm_kmalloc(sizeof(task_Union), mm_Attr_Shared, NULL);
    memset(tskUnion, 0, sizeof(task_Union));
    task_TaskStruct *tsk = &tskUnion->task;

    tsk->pid = task_pidCnt.value;
    Atomic_inc(&task_pidCnt);
    tsk->state = task_state_Idle;
    tsk->vRuntime = task_current->vRuntime;

    List_init(&task_current->scheNd);

    tsk->thread = task_current->thread;

    hal_task_newSubTask(tsk, entryAddr, arg, attr);
    
    task_insSubTask(tsk, tsk->thread);

    if (hal_task_dispatchTask(tsk) == res_FAIL) {
        printk(RED, BLACK, "task: failed to dispatch task #%ld.\n", tsk->pid);
        task_delSubTask(tsk);
        mm_kfree(tsk, mm_Attr_Shared);
        return NULL;
    }

    _insertNewTsk(tsk);

    return tsk;
}

task_TaskStruct *task_newTask(void *entryAddr, u64 arg, u64 attr) {
    task_Union *tskUnion = mm_kmalloc(sizeof(task_Union), mm_Attr_Shared, NULL);
    memset(tskUnion, 0, sizeof(task_Union));
    if (tskUnion == NULL) {
        printk(RED, BLACK, "task: failed to allocate task structure.\n");
        return NULL;
    }
    task_TaskStruct *tsk = &tskUnion->task;

    tsk->pid = task_pidCnt.value;
    Atomic_inc(&task_pidCnt);
    tsk->state = task_state_Idle;
    tsk->vRuntime = task_current->vRuntime;

    List_init(&task_current->scheNd);

    tsk->thread = task_newThread();

    if (tsk->thread == NULL) {
        mm_kfree(tskUnion, mm_Attr_Shared);
        return NULL;
    }
    
    hal_task_newTask(tsk, entryAddr, arg, attr);

    task_insSubTask(tsk, tsk->thread);

    if (hal_task_dispatchTask(tsk) == res_FAIL) {
        printk(RED, BLACK, "task: failed to dispatch task #%ld.\n", tsk->pid);
        task_delSubTask(tsk);
        mm_kfree(tsk, mm_Attr_Shared);
        return NULL;
    }
    
    _insertNewTsk(tsk);

    return tsk;
}

void task_exit(u64 res) {
    /// @todo free simd struct
    hal_task_exit(res);
    
    task_current->priority = task_Priority_Lowest;
    task_current->state = task_state_NeedFree;
    while (1) task_sche_wake(task_freeMgrTsk), task_sche_yield();
}

task_TaskStruct *task_freeMgrTsk;

u64 task_freeMgr(u64 arg) {
    u64 tot = 0;
    while (1) {
        if (SafeList_isEmpty(&task_mgr.freeTsks)) task_sche_sleep();
        task_current->priority = task_Priority_Running;
        task_TaskStruct *tsk = container(SafeList_getHead(&task_mgr.freeTsks), task_TaskStruct, scheNd);
        SafeList_del(&task_mgr.freeTsks, &tsk->scheNd);
        u64 pid = tsk->pid;
        if (task_freeTask(tsk) == res_FAIL) {
            printk(RED, BLACK, "task: failed to free task #%ld.\n", pid);
        } else printk(GREEN, BLACK, "task: task #%ld is killed, tot=%ld\n", pid, ++tot);

        task_current->priority = task_Priority_Lowest;
    }
    return 0;
}