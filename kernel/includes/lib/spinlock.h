#ifndef __LIB_SPINLOCK_H__
#define __LIB_SPINLOCK_H__

#include <lib/dtypes.h>
#include <hal/lib/spinlock.h>

#ifdef HAL_SPINLOCK_DEFINE
typedef hal_SpinLock SpinLock;

// lock and disable preempt
void SpinLock_lockMask(SpinLock *lock);
// unlock and enable preempt
void SpinLock_unlockMask(SpinLock *lock);

#define SpinLock_init hal_SpinLock_init
#define SpinLock_lock hal_SpinLock_lock
#define SpinLock_unlock hal_SpinLock_unlock
#define SpinLock_isLocked hal_SpinLock_isLocked 

#else
#error No SpinLock definition of this arch!
#endif


#endif