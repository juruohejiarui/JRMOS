#include <lib/spinlock.h>
#include <task/schedule.h>

__optimize__ void SpinLock_lockMask(SpinLock *lock) {
	task_sche_pause();
	SpinLock_lock(lock);
}

__optimize__ void SpinLock_unlockMask(SpinLock *lock) {
	SpinLock_unlock(lock);
	task_sche_resume();
}