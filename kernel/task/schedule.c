#include "inner.h"
#include <mm/mm.h>
#include <lib/string.h>
#include <screen/screen.h>
#include <hal/task/schedule.h>

const int task_sche_cfsWeight[40] = {
    /* -20 */ 88761, 71755, 56483, 46273, 36291,
    /* -15 */ 29154, 23254, 18705, 14949, 11916,
    /* -10 */  9548,  7620,  6100,  4904,  3906,
    /*  -5 */  3121,  2501,  1991,  1586,  1277,
    /*   0 */  1024,   820,   655,   526,   423,
    /*   5 */   335,   272,   215,   172,   137,
    /*  10 */   110,    87,    70,    56,    45,
    /*  15 */    36,    29,    23,    18,    15,
};

cpu_definevar(RBTree, task_sche_thds);
cpu_definevar(Atomic, task_sche_msk);
cpu_definevar(ListNode, task_sche_freeThds);
cpu_definevar(ListNode, task_sche_sleepThds);
cpu_definevar(ListNode, task_sche_rdyPreemptThds);
cpu_definevar(SafeList, task_sche_reqs);
cpu_definevar(u64, task_sche_switchCnt);

__optimize__ void task_sche_waitReq() {
    Atomic_inc(&task_cur->reqWait);
    while (task_cur->reqWait.value > 0) task_sche_yield();
}

void task_sche_finishReq(task_Thread *thd) {
    Atomic_dec(&thd->reqWait);
    if (thd->reqWait.value == 0) task_sche_preempt(thd);
}

static int _sche_enable = 0;

void task_sche_enable() { _sche_enable = 1; }

void task_sche_disable() { _sche_enable = 0; }

__always_inline__ int _sche_chkEnable() { return _sche_enable; }

int task_sche_chkEnable() { return _sche_chkEnable(); }

__optimize__ u64 task_sche_getSwitchCnt() { return cpu_getvar(task_sche_switchCnt); }

__always_inline__ int task_sche_thds_compare(RBNode *a, RBNode *b) {
    task_Thread *ta = container(a, task_Thread, rbNd), *tb = container(b, task_Thread, rbNd);
    return (ta->vRuntime != tb->vRuntime ? ta->vRuntime < tb->vRuntime : ta->pid < tb->pid);
}

RBTree_insertDef(task_sche_thds_insert, task_sche_thds_compare);

void _sche_init() {
    printk(screen_log, "task: _sche_init()\n");
    for (int i = 0; i < cpu_num; i++) {
        SafeList_init(cpu_cpuPtr(i, task_sche_reqs));
        List_init(cpu_cpuPtr(i, task_sche_freeThds));
        List_init(cpu_cpuPtr(i, task_sche_sleepThds));
        List_init(cpu_cpuPtr(i, task_sche_rdyPreemptThds));
        RBTree_init(cpu_cpuPtr(i, task_sche_thds), task_sche_thds_insert, NULL);
        cpu_cpuPtr(i, task_sche_msk)->value = 0;
        *cpu_cpuPtr(i, task_sche_switchCnt) = 0;
    }
}

__optimize__ void task_sche_updCurThdState() {
    task_Thread *cur = task_cur;
    cur->vRuntime += task_sche_cfsWeight[cur->priority + 20];
    cur->state |= task_state_NeedSchedule;
}

void task_sche_updAllCurThdsState() {
    if (__likely__(_sche_chkEnable())) {
        task_sche_updCurThdState();
        hal_task_sche_updOtherThdsState();
    }
}

void task_sche_yield() {
    task_sche_updCurThdState();
    hal_task_sche_yield();
}

// wake a proc on the current CPU, which means the proc will be scheduled immediately after being woken up
__optimize__ void _wakeThd(task_Thread *thd) {
    _sche_syncVRutnime(thd);
    switch (thd->state) {
        case task_state_Sleep :
            List_del(&thd->scheNd);
            RBTree_ins(cpu_ptr(task_sche_thds), &thd->rbNd);
            thd->state = task_state_Idle;
            break;
    }
}


// preempt a proc on the current CPU, which means the proc will be scheduled immediately after being preempted
__optimize__ void _preemptThd(task_Thread *thd) {
    _sche_syncVRutnime(thd);
    // printk(screen_log, "task: sche: _launchThd(): cpu #%d preempt task #%d:%p\n", cpu_getvar(cpu_id), thd->pid, thd);
    switch (thd->state) {
        case task_state_Idle :
            thd->state = task_state_NeedPreempt;
            RBTree_del(cpu_ptr(task_sche_thds), &thd->rbNd);
            List_insTail(cpu_ptr(task_sche_rdyPreemptThds), &thd->scheNd);
            break;
        case task_state_Sleep :
            thd->state = task_state_NeedPreempt;
            List_del(&thd->scheNd);
            List_insTail(cpu_ptr(task_sche_rdyPreemptThds), &thd->scheNd);
            break;
    }

}

__optimize__ void _launchThd(task_Thread *thd) {
    // printk(screen_log, "task: sche: _launchThd(): cpu #%d launch task #%d:%p\n", cpu_getvar(cpu_id), thd->pid, thd);
    _sche_syncVRutnime(thd);

    RBTree_ins(cpu_ptr(task_sche_thds), &thd->rbNd);
}

// if thd is on the current CPU, wake / preempt it immediately; otherwise, send a request to the target CPU to wake / preempt it
__optimize__ void task_sche_wake(task_Thread *thd) {
    if (__unlikely__(thd->cpuId == cpu_getvar(cpu_id))) {
        task_sche_pause();
        _wakeThd(thd);
        task_sche_resume();
    } else {
        WaitReq *req = mm_kmalloc(sizeof(WaitReq), mm_Attr_Shared, NULL);
        req->thd = thd;
        req->targetState = task_state_Ready;
        SafeList_insTail(cpu_cpuPtr(thd->cpuId, task_sche_reqs), &req->lstNd);
    }
}

// if thd is on the current CPU, wake / preempt it immediately; otherwise, send a request to the target CPU to wake / preempt it
__optimize__ void task_sche_preempt(task_Thread *thd) {
    if (__unlikely__(thd->cpuId == cpu_getvar(cpu_id))) {
        task_sche_pause();
        _preemptThd(thd);
        task_sche_resume();
    } else {
        WaitReq *req = mm_kmalloc(sizeof(WaitReq), mm_Attr_Shared, NULL);
        req->thd = thd;
        req->targetState = task_state_NeedPreempt;
        SafeList_insTail(cpu_cpuPtr(thd->cpuId, task_sche_reqs), &req->lstNd);
    }
}

__optimize__ void task_sche_launch(task_Thread *thd) {
    if (__unlikely__(thd->cpuId == cpu_getvar(cpu_id))) {
        task_sche_pause();
        _launchThd(thd);
        task_sche_resume();
    } else {
        WaitReq *req = mm_kmalloc(sizeof(WaitReq), mm_Attr_Shared, NULL);
        req->thd = thd;
        req->targetState = task_state_Idle;
        SafeList_insTail(cpu_cpuPtr(thd->cpuId, task_sche_reqs), &req->lstNd);
    }
}

// handle the requests sent by other CPUs to wake / preempt threads on the current CPU
__optimize__ void task_sche_handleReq() {
    while (!SafeList_isEmpty(cpu_ptr(task_sche_reqs))) {
        task_sche_pause();
        WaitReq *req = container(SafeList_delHead(cpu_ptr(task_sche_reqs)), WaitReq, lstNd);
        switch (req->targetState) {
            case task_state_NeedPreempt :
                _preemptThd(req->thd);
                break;
            case task_state_Ready :
                _wakeThd(req->thd);
                break;
            case task_state_Idle :
                _launchThd(req->thd);
                break;
        }
        task_sche_resume();
        mm_kfree(req, mm_Attr_Shared);
    }
    task_sche_pause();
    while (!List_isEmpty(cpu_ptr(task_sche_freeThds))) {
        task_Thread *thd = container(List_delHead(cpu_ptr(task_sche_freeThds)), task_Thread, scheNd);
        int res = task_freeThd(thd);
    }
    task_sche_resume();
}

// hang the current proc
__always_inline__ void _hangCurThd() {
    register task_Thread *cur = task_cur;
    if (cur->reqWait.value > 0 && cur->workingSignal == 0) {
        // printk(screen_log, "task: sche: _hangCurThd(): thread #%d sleep.\n", cur->pid);
        cur->state = task_state_Sleep;
        List_insTail(cpu_ptr(task_sche_sleepThds), &cur->scheNd);
        return ;
    }
    switch (cur->state) {
        case task_state_NeedFree :
            cur->state = task_state_Free;
            List_insTail(cpu_ptr(task_sche_freeThds), &cur->scheNd);
            break;
        default :
            cur->state = task_state_Idle;
            RBTree_ins(cpu_ptr(task_sche_thds), &cur->rbNd);
            break;
    }
}

// try to schedule another proc on the current CPU; if there is no other proc, just continue to run the current proc
__optimize__ void task_sche_trySche() {
    task_Thread *nxt;
    // RBTree_debug(cpu_ptr(task_sche_thds));
    if (__likely__(!List_isEmpty(cpu_ptr(task_sche_rdyPreemptThds)))) {
        nxt = container(List_delHead(cpu_ptr(task_sche_rdyPreemptThds)), task_Thread, scheNd);
    } else {
        RBNode *nd = RBTree_getLeft(cpu_ptr(task_sche_thds));
        if (__unlikely__(nd == NULL)) goto no_switch;
        nxt = container(nd, task_Thread, rbNd);
        if (nxt->vRuntime > task_cur->vRuntime) goto no_switch;
        RBTree_del(cpu_ptr(task_sche_thds), nd);
    }
    (*cpu_ptr(task_sche_switchCnt))++;
    nxt->state = task_state_Running;
    _hangCurThd();
    // printk(screen_log, "task: sche: task_sche_trySche(): cpu #%d: from %p to %p\n", cpu_getvar(cpu_id), task_cur, nxt);
    hal_task_sche_switch(task_cur, nxt);
    return ;
    no_switch:
    task_cur->state = task_state_Running;
}