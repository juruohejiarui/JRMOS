#ifndef __TASK_SEMAPHORE_H__
#define __TASK_SEMAPHORE_H__

#include <lib/spinlock.h>
#include <lib/list.h>

typedef struct task_Semaphore task_Semaphore;

struct task_Semaphore {
    SpinLock lck;
    int cnt;
    ListNode lst;
};

__always_inline__ void task_Semaphore_init(task_Semaphore *sem, int cnt) {
    SpinLock_init(&sem->lck);
    sem->cnt = cnt;
    List_init(&sem->lst);
}
void task_Semaphore_wait(task_Semaphore *sem);

int task_Semaphore_tryWait(task_Semaphore *sem);

void task_Semaphore_signal(task_Semaphore *sem);

#endif