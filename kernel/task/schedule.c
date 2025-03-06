#include <task/api.h>
#include <interrupt/api.h>

static task_MgrStruct task_mgr;

static u64 task_sche_cfsTbl[0x20] = {
    0x1, 0x2, 0x4, 0x8, 0x10
};

static __always_inline__ int task_sche_cfsTreeCmp(RBNode *a, RBNode *b) {
    register task_TaskStruct
        *ta = container(a, task_TaskStruct, rbNode),
        *tb = container(b, task_TaskStruct, rbNode);
    return (ta->vRuntime != tb->vRuntime ? ta->vRuntime < tb->vRuntime : (u64)ta < (u64)tb);
}

RBTree_insert(task_sche_cfsTreeIns, task_sche_cfsTreeCmp)

static __always_inline__ void task_sche_updCurState() {
    register u64 tmp = task_sche_cfsTbl[task_current->priority];
    task_current->vRuntime++;
    task_current->resRuntime--;
    if (!task_current->resRuntime)
        task_current->state = task_state_NeedSchedule;
}

static __always_inline__ void task_sche_updOtherState() {
    hal_task_sche_updOtherState();
}

void task_sche_updState() {
    task_sche_updCurState();
    task_sche_updOtherState();
}

void task_sche_init() {
    for (int i = 0; i < cpu_num; i++) {
        RBTree_init(&task_mgr.tasks[i], task_sche_cfsTreeIns);
    }
    RBTree_init(&task_mgr.freeTasks, task_sche_cfsTreeIns);
    task_mgr.freeTaskNum.value = 0;
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
    hal_task_sche_switch(nextTsk);
}