#include <task/api.h>
#include <mm/mm.h>
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
        RBTree_init(&task_mgr.tasks[i], task_sche_cfsTreeIns);
    }
    RBTree_init(&task_mgr.freeTasks, task_sche_cfsTreeIns);
    task_mgr.freeTaskNum.value = 0;
    task_sche_state = 0;
    task_pidCnt = 0;
}

void task_sche_release() {
    task_current->resRuntime = 0;
    task_current->state = task_state_NeedSchedule;
}

void task_schedule() {
    RBTree *tasks = &task_mgr.tasks[task_current->cpuId];
    task_TaskStruct *nextTsk;
    if (task_current->state == task_state_WaitFree) {
        RBTree_ins(&task_mgr.freeTasks, &task_current->rbNode);
        Atomic_inc(&task_mgr.freeTaskNum);
    } else {
        task_current->state = task_state_Idle;
        task_current->resRuntime = task_sche_cfsTbl[task_current->priority];
        RBNode *nextTskNode = RBTree_getMin(tasks);
        if (nextTskNode != NULL) {
            nextTsk = container(nextTskNode, task_TaskStruct, rbNode);
            if (nextTsk->vRuntime < task_current->vRuntime) {
                RBTree_ins(tasks, &task_current->rbNode);
                RBTree_del(tasks, nextTskNode);
            } else nextTsk = task_current;
        } else nextTsk = task_current;
    }
    printk(RED, BLACK, "T%ld\n", nextTsk->pid);
    hal_task_sche_switch(nextTsk);
}

task_ThreadStruct *task_newThread() {
    task_ThreadStruct *thread = mm_kmalloc(sizeof(task_ThreadStruct), mm_Attr_Shared, NULL);
    SpinLock_lock(&mm_map_krlTblLck);
    thread->krlTblModiJiff.value = mm_map_krlTblModiJiff.value;
    thread->allocMem.value = thread->allocVirtMem.value = 0;
    
    SpinLock_init(&thread->pageRecordLck);
    List_init(&thread->pageRecord);

    RBTree_init(&thread->slabRecord, mm_slabRecord_insert);

    memset(thread->sigHandler, 0, sizeof(thread->sigHandler));

    SpinLock_init(&thread->pgTblLck);

    List_init(&thread->tskList);

    hal_task_newThread(&thread->hal);

    SpinLock_unlock(&mm_map_krlTblLck);


    return thread;
}

void task_insSubTask(task_TaskStruct *subTsk, task_ThreadStruct *thread) {
    List_insBefore(&subTsk->list, &thread->tskList);
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
    task_insSubTask(tsk, tsk->thread);

    tsk->cpuId = hal_task_dispatchTask(tsk);

    memset(tsk->signal, 0, sizeof(tsk->signal));

    hal_task_newSubTask(tsk, entryAddr, arg, attr);

    intr_mask();
    RBTree_ins(&task_mgr.tasks[tsk->cpuId], &tsk->rbNode);
    intr_unmask();

    return tsk;
}