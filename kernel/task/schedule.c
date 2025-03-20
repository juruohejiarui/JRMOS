#include <task/api.h>
#include <mm/mm.h>
#include <mm/buddy.h>
#include <interrupt/api.h>
#include <screen/screen.h>

static task_MgrStruct task_mgr;

// this variable will be used by hal/interrupt/entry.S
int task_sche_state;
u64 task_pidCnt;

static u64 task_sche_cfsTbl[0x20] = {
    0x1, 0x2, 0x4, 0x8, 0x10
};

void task_sche_enable() { task_sche_state = 1; }
void task_sche_disable() { task_sche_state = 0; }
int task_sche_getState() { return task_sche_state; }

static __always_inline__ int task_sche_cfsTreeCmp(RBNode *a, RBNode *b) {
    register task_TaskStruct
        *ta = container(a, task_TaskStruct, rbNode),
        *tb = container(b, task_TaskStruct, rbNode);
    return (ta->vRuntime != tb->vRuntime ? ta->vRuntime < tb->vRuntime : (u64)ta < (u64)tb);
}

RBTree_insert(task_sche_cfsTreeIns, task_sche_cfsTreeCmp)

void task_sche_updCurState() {
    // printk(RED, BLACK, "C%d", task_current->cpuId);
    register u64 tmp = task_sche_cfsTbl[task_current->priority];
    task_current->vRuntime += tmp;
    task_current->resRuntime -= tmp;
    if (task_current->resRuntime <= 0)
        task_current->state = task_state_NeedSchedule;
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
    }
    RBTree_init(&task_mgr.freeTasks, task_sche_cfsTreeIns, task_sche_cfsTreeCmp);
    task_sche_state = 0;
    task_pidCnt = 0;
}

void task_sche_release() {
    task_current->resRuntime = 0;
    task_current->state = task_state_NeedSchedule;
    hal_task_sche_release();
}

void task_schedule() {
    RBTree *tasks = &task_mgr.tasks[task_current->cpuId];
    RBNode *nextTskNode = RBTree_getLeft(tasks);
    // printk(WHITE, BLACK, "nexTsk:%#018lx\t", nextTskNode);
    task_TaskStruct *nextTsk;
    if (task_current->flag & task_flag_WaitFree) {
        RBTree_ins(&task_mgr.freeTasks, &task_current->rbNode);
        nextTsk = container(nextTskNode, task_TaskStruct, rbNode);
        RBTree_del(tasks, nextTskNode);
    } else {
        task_current->resRuntime = task_sche_cfsTbl[task_current->priority];
        if (nextTskNode != NULL) {
            nextTsk = container(nextTskNode, task_TaskStruct, rbNode);
            if (nextTsk->vRuntime < task_current->vRuntime) {
                task_current->state = task_state_Idle;
                RBTree_del(tasks, nextTskNode);
                RBTree_ins(tasks, &task_current->rbNode);
            } else nextTsk = task_current;
        } else nextTsk = task_current;
    }
    nextTsk->state = task_state_Running;
    hal_task_sche_switch(nextTsk);
}

task_ThreadStruct *task_newThread() {
    task_ThreadStruct *thread = mm_kmalloc(sizeof(task_ThreadStruct), mm_Attr_Shared, NULL);
    SpinLock_lock(&mm_map_krlTblLck);
    thread->krlTblModiJiff.value = mm_map_krlTblModiJiff.value;
    thread->allocMem.value = thread->allocVirtMem.value = 0;
    
    SpinLock_init(&thread->pageRecordLck);
    List_init(&thread->pgRecord);

    RBTree_init(&thread->slabRecord, mm_slabRecord_insert, mm_slabRecord_comparator);

    memset(thread->sigHandler, 0, sizeof(thread->sigHandler));

    SpinLock_init(&thread->pgTblLck);

    List_init(&thread->tskList);

    hal_task_newThread(&thread->hal);

    SpinLock_unlock(&mm_map_krlTblLck);

    return thread;
}

extern int task_freeThread(task_ThreadStruct *thread);
extern int task_freeTask(task_TaskStruct *task);

void task_insSubTask(task_TaskStruct *subTsk, task_ThreadStruct *thread) {
    SpinLock_lock(&thread->tskListLck);
    List_insBefore(&subTsk->list, &thread->tskList);
    SpinLock_unlock(&thread->tskListLck);
}

int task_delSubTask(task_TaskStruct *subTsk) {
    task_ThreadStruct *thread = subTsk->thread;
    SpinLock_lock(&thread->tskListLck);
    List_del(&subTsk->list);
    if (List_isEmpty(&thread->tskList)) {
        SpinLock_unlock(&thread->tskListLck);
        return task_freeThread(thread);
    } else SpinLock_unlock(&thread->tskListLck);
    return res_SUCC;
}

int task_freeThread(task_ThreadStruct *thread) {
    for (List *pageList = thread->pgRecord.next; pageList != &thread->pgRecord; ) {
        List *nxt = pageList->next;
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
    if (hal_task_freeTask(tsk) == res_FAIL) return res_FAIL;
    return mm_kfree(tsk, mm_Attr_Shared);
}

void task_initIdle() {
    task_current->pid = task_pidCnt++;
    cpu_Desc *curCpu = &cpu_desc[task_current->cpuId];
    task_current->priority = 0;
    task_current->state = 0;

    task_current->resRuntime = 1;
    task_current->vRuntime = 0;

    memset(task_current->signal, 0, sizeof(task_current->signal));

    task_current->thread = task_newThread();
    task_insSubTask(task_current, task_current->thread);

    hal_task_initIdle();
}

task_TaskStruct *task_newSubTask(void *entryAddr, u64 arg, u64 attr) {
    task_Union *tskUnion = mm_kmalloc(sizeof(task_Union), mm_Attr_Shared, NULL);
    task_TaskStruct *tsk = &tskUnion->task;
    tsk->pid = task_pidCnt++;
    tsk->priority = 0;
    tsk->state = 0;
    tsk->resRuntime = 1;
    tsk->vRuntime = 0;

    tsk->thread = task_current->thread;

    memset(tsk->signal, 0, sizeof(tsk->signal));

    hal_task_newSubTask(tsk, entryAddr, arg, attr);
    
    task_insSubTask(tsk, tsk->thread);

    tsk->cpuId = hal_task_dispatchTask(tsk);

    intr_mask();
    RBTree_ins(&task_mgr.tasks[tsk->cpuId], &tsk->rbNode);
    intr_unmask();

    return tsk;
}

void task_exit(u64 res) {
    /// @todo free simd struct

    hal_task_exit(res);

	hal_task_current->flag |= task_flag_WaitFree;

    while (1) task_sche_release();
}

u64 task_freeMgr(u64 arg) {
    while (1) {
        // printk(WHITE, BLACK, "task: freeMgr: arg=%#018lx\n", arg);
        task_TaskStruct *tsk = NULL;
        intr_mask();
        {
            RBNode *tskNode = RBTree_getLeft(&task_mgr.freeTasks);
            if (tskNode) {
                tsk = container(tskNode, task_TaskStruct, rbNode);
                RBTree_del(&task_mgr.freeTasks, tskNode);
            }
        }
        intr_unmask();
        if (tsk == NULL) { 
            task_sche_release();
            continue;
        }
        u64 pid = tsk->pid;
        if (task_freeTask(tsk) == res_FAIL) {
            printk(RED, BLACK, "task: failed to free task #%ld.\n", pid);
        } else printk(GREEN, BLACK, "task: task #%ld is killed\n", pid);
        task_sche_release();
    }
    return 0;
}