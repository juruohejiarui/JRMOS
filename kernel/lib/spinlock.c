#include <lib/spinlock.h>
#include <task/api.h>

__optimize__ void SpinLock_lockMask(SpinLock *lock) {
	task_sche_msk();
	SpinLock_lock(lock);
}

__optimize__ void SpinLock_unlockMask(SpinLock *lock) {
	SpinLock_unlock(lock);
	task_sche_unmsk();
}