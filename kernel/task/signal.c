#include "inner.h"
#include <interrupt/api.h>
#include <screen/screen.h>
#include <mm/mm.h>

void task_signal_setHandler(u64 signal, void (*handler)(u64), u64 param) {
    task_cur->thread->sigHandler[signal] = handler;
    task_cur->thread->sigParam[signal] = param;
    printk(screen_log, "task: signal: task #%d set handler for signal #%d with param %p\n", task_cur->pid, signal, param);
}

void task_signal_send(task_Thread *target, u64 signal) {
	Atomic_bts(&target->signal, signal);
	task_sche_wake(target);
}

// request of sending signal from interrupt program
struct task_signal_Request {
    task_Thread *target;
    u64 signal;
    ListNode lst;
};

static SafeList task_signal_reqLst;

void task_signal_sendFromIntr(task_Thread *target, u64 signal) {
    struct task_signal_Request *req = mm_kmalloc(sizeof(struct task_signal_Request), mm_Attr_Shared, NULL);
    req->target = target;
    req->signal = signal;
    SafeList_insTail(&task_signal_reqLst, &req->lst);
    task_sche_wake(_mgrThd);
    printk(screen_log, "task: signal: send signal %d to #%ld from interrupt program\n", req->signal, req->target->pid);
}

void task_signal_mgrTskMain() {
    while (1) {
        while (SafeList_isEmpty(&task_signal_reqLst)) 
        task_cur->priority = task_Priority_Running;
        while (!SafeList_isEmpty(&task_signal_reqLst)) {
            struct task_signal_Request *req = container(SafeList_delHead(&task_signal_reqLst), struct task_signal_Request, lst);
            printk(screen_log, "task: signal: send signal %d to #%ld\n", req->signal, req->target->pid);
            task_signal_send(req->target, req->signal);
        }
        task_cur->priority = task_Priority_Lowest;
    }
}

__optimize__ void task_signal_scan() {
    task_Thread *cur = task_cur;
    printk(screen_log, "task: signal: task #%ld scanning on %#018lx\n", cur->pid, cur->signal.value);
    for (int i = 0; i < 64; i++) if (cur->signal.value & (1ul << i)) {
        void (*handler)(u64);
        if (!(handler = cur->thread->sigHandler[i])) {
            printk(screen_err, "task: signal: task %ld no handler for signal #%d\n", cur->pid, i);
            if (!i) task_exit(-1);
        } else {
            // if there is handler, then call the handler and reset the bit
            Atomic_btr(&cur->signal, i);
            cur->signalHandle++;
            intr_unmask();
            printk(screen_log, "task: singal: task %ld handle signal #%d\n", cur->pid, i);
            handler(cur->thread->sigParam[i]);
            intr_mask();
            cur->signalHandle--;
        }
    }
}

void _signal_init() {
    SafeList_init(&task_signal_reqLst);
    _mgrThd = task_newSubTask(task_signal_mgrTskMain, 0, task_attr_Builtin);
}