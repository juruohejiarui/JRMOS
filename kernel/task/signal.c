#include "inner.h"
#include <interrupt/api.h>
#include <screen/screen.h>
#include <mm/mm.h>

void task_signal_setHandler(u64 signal, void (*handler)(i64), u64 param) {
    task_cur->proc->sigHandler[signal] = handler;
    task_cur->proc->sigParam[signal] = param;
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

__optimize__ void task_signal_scan() {
    task_Thread *cur = task_cur;
    printk(screen_log, "task: signal: task #%ld scanning on %#018lx\n", cur->pid, cur->signal.value);
    for (int i = 0; i < 64; i++) if (cur->signal.value & (1ul << i)) {
        task_SignalHandler handler;
        if (!(handler = cur->proc->sigHandler[i])) {
            printk(screen_err, "task: signal: task %ld no handler for signal #%d\n", cur->pid, i);
            if (!i) task_exit(-1);
        } else {
            // if there is handler, then call the handler and reset the bit
            Atomic_btr(&cur->signal, i);
            cur->workingSignal++;
            intr_unmask();
            printk(screen_log, "task: singal: task %ld handle signal #%d\n", cur->pid, i);
            handler(cur->proc->sigParam[i]);
            intr_mask();
            cur->workingSignal--;
        }
    }
}

void _signal_init() {
}