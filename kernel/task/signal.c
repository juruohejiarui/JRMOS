#include <task/api.h>
#include <interrupt/api.h>
#include <screen/screen.h>
#include <mm/mm.h>

void task_signal_setHandler(u64 signal, void (*handler)(u64), u64 param) {
    task_current->thread->sigHandler[signal] = handler;
    task_current->thread->sigParam[signal] = param;
    printk(screen_log, "task: signal: task #%d set handler for signal #%d with param %p\n", task_current->pid, signal, param);
}

void task_signal_send(task_TaskStruct *target, u64 signal) {
	Atomic_bts(&target->signal, signal);
	task_sche_wake(target);
}

// request of sending signal from interrupt program
struct task_signal_Request {
    task_TaskStruct *target;
    u64 signal;
    ListNode lst;
};

task_TaskStruct *task_signal_mgrTsk;
static SafeList task_signal_reqLst;

void task_signal_sendFromIntr(task_TaskStruct *target, u64 signal) {
    struct task_signal_Request *req = mm_kmalloc(sizeof(struct task_signal_Request), mm_Attr_Shared, NULL);
    req->target = target;
    req->signal = signal;
    SafeList_insTail(&task_signal_reqLst, &req->lst);
}

void task_signal_mgrTskMain() {
    while (1) {
        while (SafeList_isEmpty(&task_signal_reqLst)) task_sche_yield();
        task_current->priority = task_Priority_Running;
        while (!SafeList_isEmpty(&task_signal_reqLst)) {
             struct task_signal_Request *req = container(SafeList_delHead(&task_signal_reqLst), struct task_signal_Request, lst);
            task_signal_send(req->target, req->signal);
        }
        task_current->priority = task_Priority_Lowest;
    }
}

void task_signal_scan() {
    for (int i = 0; i < 64; i++) if (task_current->signal.value & (1ul << i)) {
        void (*handler)(u64);
        if (!(handler = task_current->thread->sigHandler[i])) {
            printk(screen_err, "task: signal: task %ld no handler for signal #%d\n", task_current->pid, i);
            if (!i) task_exit(-1);
        } else {
            // if there is handler, then call the handler and reset the bit
            intr_unmask();
            Atomic_btr(&task_current->signal, i);
            printk(screen_warn, "task: singal: task %ld handle signal #%d\n", task_current->pid, i);
            handler(task_current->thread->sigParam[i]);
            intr_mask();
        }
    }
}

void task_signal_init() {
    SafeList_init(&task_signal_reqLst);
    task_newSubTask(task_signal_mgrTskMain, 0, task_attr_Builtin);
}