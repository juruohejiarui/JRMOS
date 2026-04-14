#include "inner.h"
#include <screen/screen.h>
#include <mm/mm.h>
#include <mm/mgr.h>
#include <mm/buddy.h>
#include <lib/string.h>

Atomic task_pidCnt, task_threadCnt;

SafeList task_freeTsks, task_sleepTsks;

task_ThreadStruct *task_rootThread;

task_ThreadStruct *task_newThread(u64 attr) {
    task_ThreadStruct *thread = mm_kmalloc(sizeof(task_ThreadStruct), mm_Attr_Shared, NULL);
    if (thread == NULL) {
        printk(screen_err, "task: failed to allocate thread structure.\n");
        return NULL;
    }
    memset(thread, 0, sizeof(task_ThreadStruct));

    thread->id = task_threadCnt.value;
    Atomic_inc(&task_threadCnt);
    thread->state = task_state_Running;

    mm_mgr_init(&thread->mem, attr);

    memset(thread->sigHandler, 0, sizeof(thread->sigHandler));

    SafeList_init(&thread->tskList);
    SafeList_init(&thread->children);

    SafeList_init(&thread->fileLst);
    SafeList_init(&thread->dirLst);

    if (~attr & task_attr_Root) {
        SafeList_insTail(&task_cur->thread->children, &thread->childNd);
        thread->parent = task_cur->thread;
    } else {
        thread->parent = NULL;
    }

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

    task_cur->thread = task_rootThread;

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

void task_init() {
    task_sche_init();

    task_rootThread = task_newThread(task_attr_Builtin | task_attr_Root);
}