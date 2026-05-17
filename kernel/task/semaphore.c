#include <task/semaphore.h>
#include "inner.h"

void task_Semaphore_wait(task_Semaphore *sem) {
    SpinLock_lockMask(&sem->lck);
    if (sem->cnt-- <= 0) {
        List_insTail(&sem->lst, &task_cur->semNd);
        task_sche_waitReq();
        SpinLock_unlockMask(&sem->lck);
        task_sche_yield();
    } else SpinLock_unlockMask(&sem->lck);
}

int task_Semaphore_tryWait(task_Semaphore *sem) {
    SpinLock_lockMask(&sem->lck);
    if (sem->cnt-- <= 0) {
        SpinLock_unlockMask(&sem->lck);
        return 0;
    } else SpinLock_unlockMask(&sem->lck);
    return 1;
}
void task_Semaphore_signal(task_Semaphore *sem) {
    SpinLock_lockMask(&sem->lck);
    sem->cnt++;
    if (!List_isEmpty(&sem->lst)) {
        task_Thread *tsk = container(List_delHead(&sem->lst), task_Thread, semNd);
        SpinLock_unlockMask(&sem->lck);
        task_sche_preempt(tsk);
    } else {
        SpinLock_unlockMask(&sem->lck);
    }
}