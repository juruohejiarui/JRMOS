#ifndef __HAL_LIB_ATOMIC_H__
#define __HAL_LIB_ATOMIC_H__

#include <lib/dtypes.h>

typedef struct {
	volatile i64 value;
} hal_Atomic;

#define HAL_LIB_ATOMIC_ADD
#define HAL_LIB_ATOMIC_SUB
#define HAL_LIB_ATOMIC_INC
#define HAL_LIB_ATOMIC_DEC

static __always_inline__ void hal_Atomic_add(hal_Atomic *atomic, i64 vl) {
	__asm__ volatile ( " lock addq %1, %0	\n\t"
		: "=m"(atomic->value) : "r"(vl) : "memory");
}

static __always_inline__ void hal_Atomic_sub(hal_Atomic *atomic, i64 vl) {
	__asm__ volatile ( " lock subq %1, %0	\n\t"
		: "=m"(atomic->value) : "r"(vl) : "memory");
}

static __always_inline__ void hal_Atomic_inc(hal_Atomic *atomic) {
	__asm__ volatile ( " lock incq %0	\n\t"
		: "=m"(atomic->value) : "m"(atomic->value) : "memory");
}

static __always_inline__ void hal_Atomic_dec(hal_Atomic *atomic) {
	__asm__ volatile ( " lock decq %0	\n\t"
		: "=m"(atomic->value) : "m"(atomic->value) : "memory");
}

#endif