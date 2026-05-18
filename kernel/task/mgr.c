#include "inner.h"
#include <mm/mm.h>
#include <mm/mgr.h>
#include <lib/string.h>
#include <screen/screen.h>
#include <hal/task/schedule.h>

Atomic task_thdCnt, task_procCnt;

task_Process *task_rootProc;

cpu_definevar(task_Thread *, task_idleThd);
cpu_definevar(task_Thread *, task_curThd);

void _mgr_init() {
    task_thdCnt.value = task_procCnt.value = 0;

    task_rootProc = task_newProc(task_attr_Root | task_attr_Builtin);
}

__always_inline__ void _addNewThd(task_Process *proc, task_Thread *thd) {
    printk(screen_log, "task: mgr: _addNewThd(): proc #%d add thread #%d:%p\n", proc->id, thd->pid, thd);
    SafeList_insTail(&proc->thdLst, &thd->thdNd);
    thd->proc = proc;
    if (SafeList_getHead(&proc->thdLst) == &thd->thdNd)
        proc->mainThd = thd;
}

__always_inline__ void _delThd(task_Process *proc, task_Thread *thd) {
    printk(screen_log, "task: mgr: _delThd(): proc #%d del thread #%d:%p\n", proc->id, thd->pid, thd);
    SafeList_del(&proc->thdLst, &thd->thdNd);
}
task_Thread *task_newThd(void *entryAddr, void *arg, u64 attr, task_Process *proc) {
    task_Union *uPtr = mm_kmalloc(sizeof(task_Union), mm_Attr_Shared, NULL);
    if (uPtr == NULL) {
        printk(screen_err, "task: mgr: task_newThd(): failed to allocate memory for new thread.\n");
        return NULL;
    }
    task_Thread *thd = &uPtr->thd;

    memset(uPtr, 0, sizeof(task_Union));
    
    if (hal_task_sche_dispatch(thd) == res_FAIL) {
        mm_kfree(uPtr, mm_Attr_Shared);
        return NULL;
    }
    _addNewThd(proc, thd);

    thd->priority = 0;
    thd->pid = task_thdCnt.value;
    thd->state = task_state_Idle;

    _sche_syncVRutnime(thd);

    List_init(&thd->scheNd);

    Atomic_inc(&task_thdCnt);

    hal_task_newThd(thd, entryAddr, arg, attr);

    return thd;
}

task_Process *task_newProc(u64 attr) {
    task_Process *proc = mm_kmalloc(sizeof(task_Process), mm_Attr_Shared, NULL);
    if (proc == NULL) {
        printk(screen_err, "task: mgr: task_newProc(): failed to allocate memory for new process.\n");
        return NULL;
    }
    memset(proc, 0, sizeof(task_Process));

    proc->id = task_procCnt.value;
    proc->state = task_state_Running;

    mm_mgr_init(&proc->mem, attr);

    memset(proc->sigHandler, 0, sizeof(proc->sigHandler));
    memset(proc->sigParam, 0, sizeof(proc->sigParam));

    SafeList_init(&proc->thdLst);
    SafeList_init(&proc->children);

    SafeList_init(&proc->fileLst);
    SafeList_init(&proc->dirLst);

    if (~attr & task_attr_Root) {
        SafeList_insTail(&task_rootProc->children, &proc->childNd);
        proc->parent = task_rootProc;
    } else {
        proc->parent = NULL;
    }

    hal_task_newProc(proc, attr);
    
    return proc;
}

void task_exit(i64 res) {
    task_sche_pause();
    task_cur->state = task_state_NeedFree;
    task_sche_resume();
    while (1) task_sche_yield();
}

void task_initIdleThd() {
    cpu_setvar(task_idleThd, task_cur);
    cpu_setvar(task_curThd, task_cur);

    task_cur->pid = task_thdCnt.value;
    task_cur->priority = 0;
    task_cur->state = task_state_Running;
    task_cur->flag = 0;

    Atomic_inc(&task_thdCnt);

    task_cur->vRuntime = 0;

    memset(&task_cur->signal, 0, sizeof(task_cur->signal));

    _addNewThd(task_rootProc, task_cur);

    List_init(&task_cur->scheNd);

    hal_task_initIdleThd();

    printk(screen_log, "task: mgr: task_initIdleThd(): initialize idle task of cpu %d\n", cpu_getvar(cpu_id));
}

void _setZombie(task_Process *proc) {
    proc->state = task_state_Zombie;
    if (proc->parent) {
        
    }
}

int task_freeProc(task_Process *proc) {

}

int task_freeThd(task_Thread *thd) {
    task_Process *proc = thd->proc;
    
    _delThd(proc, thd);

    hal_task_freeThd(thd);

    if (proc->mainThd == thd) _setZombie(proc);
    return res_SUCC;
}