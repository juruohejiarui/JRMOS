#include "inner.h"
#include <mm/mm.h>
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

cpu_definevar(RBNode, task_sche_thds);
cpu_definevar(Atomic, task_sche_msk);
cpu_definevar(ListNode, task_sche_sleepThds);
cpu_definevar(ListNode, task_sche_rdyPreemptThds);
cpu_definevar(SafeList, task_sche_reqs);

__always_inline__ int task_sche_thds_compare(RBNode *a, RBNode *b) {
    task_Thread *ta = container(a, task_Thread, rbNd), *tb = container(b, task_Thread, rbNd);
    return (ta->vRuntime != tb->vRuntime ? ta->vRuntime < tb->vRuntime : ta->pid < tb->pid);
}

RBTree_insertDef(task_sche_thds_insert, task_sche_thds_compare);

void _sche_init() {
    for (int i = 0; i < cpu_num; i++) {
        SafeList_init(cpu_cpuPtr(i, task_sche_reqs));
        List_init(cpu_cpuPtr(i, task_sche_sleepThds));
        List_init(cpu_cpuPtr(i, task_sche_rdyPreemptThds));
        RBTree_init(cpu_cpuPtr(i, task_sche_thds), task_sche_thds_insert, NULL);
        cpu_cpuPtr(i, task_sche_msk)->value = 0;
    }
    SafeList_init(&task_freeThds);
    SafeList_init(&task_sleepThds);
}

void task_sche_updAllCurThdsState() {
    if (__likely__(task_sche_chkEnable())) {
        task_sche_updCurThdState();
        hal_task_sche_updOtherThdsState();
    }
}

void task_sche_yield() {
    task_sche_updCurThdState();
    hal_task_sche_yield();
}

// wake a thread on the current CPU
__optimize__ void _wakeThd(task_Thread *thd) {
    switch (thd->state) {
        case task_state_Sleep :
            List_del(&thd->scheNd);
            RBTree_ins(cpu_ptr(task_sche_thds), &thd->rbNd);
            thd->state = task_state_Idle;
            break;
    }
}

__optimize__ void _preemptThd(task_Thread *thd) {
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

// weak specific task immediately
void task_sche_wake(task_Thread *thd) {
    if (__unlikely__(thd->cpuId == cpu_getvar(cpu_id))) {
        task_sche_pause();
        _wakeThd(thd);
        task_sche_resume();
    } else {
        WaitReq *req = mm_kmalloc(sizeof(WaitReq), mm_Attr_Shared, NULL);
        req->thd = thd;
        req->targetState = task_state_Idle;
        SafeList_insTail(cpu_cpuPtr(thd->cpuId, task_sche_reqs), &req->lstNd);
    }
    
}

void task_sche_preempt(task_Thread *thd) {
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

void task_sche_handleReq() {
    while (!SafeList_isEmpty(cpu_ptr(task_sche_reqs))) {
        task_sche_pause();
        WaitReq *req = container(SafeList_delHead(cpu_ptr(task_sche_reqs)), WaitReq, lstNd);
        switch (req->targetState) {
            case task_state_NeedPreempt :
                _preemptThd(req->thd);
                break;
            case task_state_Idle :
                _wakeThd(req->thd);
                break;
        }
        mm_kfree(req, mm_Attr_Shared);
        task_sche_resume();
    }
}

__always_inline__ void _hangCurThd() {
    register task_Thread *cur = task_cur;
    if (cur->reqWait.value > 0) {
        cur->state = task_state_Sleep;
        List_insTail(cpu_ptr(task_sche_sleepThds), &cur->scheNd);
        return ;
    }
    switch (cur->state) {
        case task_state_NeedFree :
            cur->state = task_state_Free;
            SafeList_insTail(&task_freeThds, &cur->scheNd);
            task_sche_preempt(_freeMgrThd);
            break;
        default :
            cur->state = task_state_Idle;
            RBTree_ins(cpu_ptr(task_sche_thds), &cur->rbNd);
            break;
    }
}

void task_sche_trySche() {
    task_Thread *nxt;
    if (__likely__(List_isEmpty(cpu_ptr(task_sche_rdyPreemptThds)))) {
        nxt = container(List_delHead(cpu_ptr(task_sche_rdyPreemptThds)), task_Thread, scheNd);
    } else {
        RBNode *nd = RBTree_getLeft(cpu_ptr(task_sche_thds));
        if (__unlikely__(nd == NULL)) goto no_switch;
        RBTree_del(cpu_ptr(task_sche_thds), nd);
        nxt = container(nd, task_Thread, rbNd);
    }
    nxt->state = task_state_Running;
    _hangCurThd();
    hal_task_sche_switch(task_cur, nxt);
    return ;
    no_switch:
    task_cur->state = task_state_Running;
}