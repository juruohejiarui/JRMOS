#ifndef __HAL_LIB_SPINLOCK_H__
#define __HAL_LIB_SPINLOCK_H__

#include <lib/dtypes.h>

#define HAL_SPINLOCK_DEFINE

struct hal_SpinLock {
	volatile i64 lock;
};

typedef struct hal_SpinLock hal_SpinLock;

#define hal_SpinLock_init(locker) do { \
	(locker)->lock = 0; \
} while(0)

#define hal_SpinLock_lock(locker) do { \
	i64 a = 0, c = 1; \
	__asm__ volatile ( \
		"pushq %%rax	\n\t" \
		"pushq %%rcx	\n\t" \
		"1:				\n\t" \
		"movq %1, %%rcx	\n\t" \
		"movq %2, %%rax	\n\t" \
		"lock cmpxchg %%rcx, %0	\n\t" \
		"jne 1b			\n\t" \
		"popq %%rcx		\n\t" \
		"popq %%rax		\n\t" \
		: "=m"((locker)->lock) \
		: "r"(c), "r"(a) \
		: "%rcx", "%rax" \
	); \
} while (0)

#define hal_SpinLock_unlock(locker) do { \
	__asm__ volatile ( \
		"movq $0, %0	\n\t" \
		: "+r"((locker)->lock) \
		: \
		: "memory" \
	); \
} while (0)

#endif