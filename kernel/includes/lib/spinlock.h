#ifndef __LIB_SPINLOCK_H__
#define __LIB_SPINLOCK_H__

#include <lib/dtypes.h>
#include <hal/lib/spinlock.h>

#ifdef HAL_SPINLOCK_DEFINE
typedef hal_SpinLock SpinLock;

#define SpinLock_init hal_SpinLock_init
#define SpinLock_lock hal_SpinLock_lock
#define SpinLock_unlock hal_SpinLock_unlock

#else
#error No SpinLock definition of this arch!
#endif


#endif