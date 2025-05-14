#include <task/api.h>
#include <interrupt/api.h>
#include <screen/screen.h>

void task_signal_setHandler(u64 signal, void (*handler)(u64), u64 param) {
    task_current->thread->sigHandler[signal] = handler;
    task_current->thread->sigParam[signal] = param;
    printk(WHITE, BLACK, "task: signal: task #%d set handler for signal #%d with param %#018lx\n", task_current->pid, signal, param);
}

void task_signal_send(task_TaskStruct *target, u64 signal) {
    Atomic_bts(&target->signal, signal);
}

void task_signal_scan() {
    for (int i = 0; i < 64; i++) if (task_current->signal.value & (1ul << i)) {
        void (*handler)(u64);
        if (!(handler = task_current->thread->sigHandler[i])) {
            printk(RED, BLACK, "task: signal: no handler for signal #%d\n", i);
            if (!i) task_exit(-1);
        } else {
            // if there is handler, then call the handler and reset the bit
            intr_unmask();
            Atomic_btr(&task_current->signal, i);
            handler(task_current->thread->sigParam[i]);
            intr_mask();
        }
    }
}