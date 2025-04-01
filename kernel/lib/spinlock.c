#include <lib/spinlock.h>
#include <task/api.h>
#include <cpu/api.h>

void SpinLock_lockMask(SpinLock *lock) {
	task_sche_unmsk();
	SpinLock_lock(lock);
}

void SpinLock_unlockMask(SpinLock *lock) {
	SpinLock_unlock(lock);
	task_sche_msk();
}