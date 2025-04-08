#include <task/api.h>
#include <interrupt/api.h>
#include <screen/screen.h>

void task_signal_setHandler(u64 signal, void (*handler)(u64)) {
    task_current->thread->sigHandler[signal] = handler;
}

void task_signal_set(task_TaskStruct *target, u64 signal) {
    Atomic_bts(&task_current->signal, signal);
}

void task_signal_scan() {
    for (int i = 0; i < 64; i++) if (task_current->signal.value & (1ul << i)) {
        Atomic_btr(&task_current->signal, i);
        void (*handler)(u64);
        if (!(handler = task_current->thread->sigHandler[i])) {
            printk(RED, BLACK, "task: signal: no handler for signal #%d\n", i);
            intr_unmask();
            task_exit(-1);
        } else {
            intr_unmask();
            handler(i);
            intr_mask();
        }
    }
}