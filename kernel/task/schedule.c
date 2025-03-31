#include <task/api.h>
#include <mm/mm.h>
#include <mm/buddy.h>
#include <interrupt/api.h>
#include <screen/screen.h>
#include <cpu/api.h>
#include <lib/bit.h>

static task_MgrStruct task_mgr;

// this variable will be used by hal/interrupt/entry.S
int task_sche_state;
u64 task_pidCnt;

static u64 task_sche_cfsTbl[0x20] = {
    0x2, 0x4, 0x8, 0x10, 0x20
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
    register u64 tmp = task_sche_cfsTbl[task_current->priority];
    task_current->vRuntime += tmp;
    if (cpu_getvar(tskTree)->left != NULL
        && task_current->vRuntime > container(cpu_getvar(tskTree)->left, task_TaskStruct, rbNode)->vRuntime)
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
        RBTree_init(&task_mgr.preemptTasks[i], task_sche_cfsTreeIns, task_sche_cfsTreeCmp);
        cpu_desc[i].tskTree = &task_mgr.tasks[i];
        cpu_desc[i].preemptTskTree = &task_mgr.preemptTasks[i];
    }
    RBTree_init(&task_mgr.freeTasks, task_sche_cfsTreeIns, task_sche_cfsTreeCmp);
    task_sche_state = 0;
    task_pidCnt = 0;
}

void task_sche_yield() {
    task_current->vRuntime += task_sche_cfsTbl[task_current->priority] >> 1;
    task_current->state = task_state_NeedSchedule;
    hal_task_sche_yield();
}

int cnt = 0;
void task_schedule() {
    RBTree *tasks = cpu_getvar(preemptTskTree);
    RBNode *nextTskNode = RBTree_getLeft(tasks);
    task_TaskStruct *nextTsk;
    if (nextTskNode != NULL) {
        printk(WHITE, BLACK, "...");
        __asm__ volatile ("hlt");
        nextTsk = container(nextTskNode, task_TaskStruct, rbNode);
        RBTree_del(tasks, nextTskNode);
        tasks = task_current->flag & task_flag_WaitFree ? &task_mgr.freeTasks : cpu_getvar(tskTree);
        RBTree_ins(tasks, &task_current->rbNode);
    } else {
        tasks = cpu_getvar(tskTree);
        nextTskNode = RBTree_getLeft(tasks);
        if (task_current->flag & task_flag_WaitFree) {
            RBTree_ins(&task_mgr.freeTasks, &task_current->rbNode);
            nextTsk = container(nextTskNode, task_TaskStruct, rbNode);
            RBTree_del(tasks, nextTskNode);
        } else {
            if (nextTskNode != NULL) {
                nextTsk = container(nextTskNode, task_TaskStruct, rbNode);
                if (nextTsk->vRuntime < task_current->vRuntime) {
                    task_current->state = task_state_Idle;
                    RBTree_del(tasks, nextTskNode);
                    RBTree_ins(tasks, &task_current->rbNode);
                } else nextTsk = task_current;
            } else nextTsk = task_current;
        }
    }
    nextTsk->state = task_state_Running;
    hal_task_sche_switch(nextTsk);
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
    
    SpinLock_init(&thread->pageRecordLck);
    List_init(&thread->pgRecord);

    RBTree_init(&thread->slabRecord, mm_slabRecord_insert, mm_slabRecord_comparator);

    memset(thread->sigHandler, 0, sizeof(thread->sigHandler));

    SpinLock_init(&thread->pgTblLck);

    SpinLock_init(&thread->tskListLck);
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
    task_current->priority = 0;
    task_current->state = 0;
    task_current->flag = 0;

    task_current->vRuntime = 0;

    memset(&task_current->signal, 0, sizeof(task_current->signal));

    task_current->thread = task_newThread();
    task_insSubTask(task_current, task_current->thread);

    hal_task_initIdle();
}

task_TaskStruct *task_newSubTask(void *entryAddr, u64 arg, u64 attr) {
    task_Union *tskUnion = mm_kmalloc(sizeof(task_Union), mm_Attr_Shared, NULL);
    memset(tskUnion, 0, sizeof(task_Union));
    task_TaskStruct *tsk = &tskUnion->task;

    tsk->pid = task_pidCnt++;
    tsk->priority = 0;
    tsk->state = 0;
    tsk->vRuntime = task_current->vRuntime;

    tsk->thread = task_current->thread;

    memset(&tsk->signal, 0, sizeof(tsk->signal));

    hal_task_newSubTask(tsk, entryAddr, arg, attr);
    
    task_insSubTask(tsk, tsk->thread);

    if (hal_task_dispatchTask(tsk) == res_FAIL) {
        printk(RED, BLACK, "task: failed to dispatch task #%ld.\n", tsk->pid);
        task_delSubTask(tsk);
        mm_kfree(tsk, mm_Attr_Shared);
        return NULL;
    }

    intr_mask();
    RBTree_ins(&task_mgr.tasks[tsk->cpuId], &tsk->rbNode);
    intr_unmask();

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
    tsk->pid = task_pidCnt++;
    tsk->priority = 0;
    tsk->state = 0;
    tsk->vRuntime = task_current->vRuntime;

    tsk->thread = task_newThread();

    if (tsk->thread == NULL) {
        mm_kfree(tskUnion, mm_Attr_Shared);
        return NULL;
    }

    memset(&tsk->signal, 0, sizeof(tsk->signal));
    
    hal_task_newTask(tsk, entryAddr, arg, attr);

    task_insSubTask(tsk, tsk->thread);

    if (hal_task_dispatchTask(tsk) == res_FAIL) {
        printk(RED, BLACK, "task: failed to dispatch task #%ld.\n", tsk->pid);
        task_delSubTask(tsk);
        mm_kfree(tsk, mm_Attr_Shared);
        return NULL;
    }
    
    intr_mask();
    RBTree_ins(&task_mgr.tasks[tsk->cpuId], &tsk->rbNode);
    intr_unmask();

    return tsk;
}

void task_exit(u64 res) {
    intr_mask();
    /// @todo free simd struct

    hal_task_exit(res);

	hal_task_current->flag |= task_flag_WaitFree;

    intr_unmask();
    while (1) task_sche_yield();
}

u64 task_freeMgr(u64 arg) {
    u64 tot = 0;
    while (1) {
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
            task_sche_yield();
            continue;
        }
        u64 pid = tsk->pid;
        if (task_freeTask(tsk) == res_FAIL) {
            printk(RED, BLACK, "task: failed to free task #%ld.\n", pid);
        } else printk(GREEN, BLACK, "task: task #%ld is killed, tot=%ld\n", pid, ++tot);
        task_sche_yield();
    }
    return 0;
}