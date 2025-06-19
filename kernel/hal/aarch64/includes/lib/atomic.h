#ifndef __HAL_LIB_ATOMIC_H__
#define __HAL_LIB_ATOMIC_H__

#include <lib/dtypes.h>
#include <hal/flags.h>

typedef struct hal_Atomic {
	volatile i64 value;
} hal_Atomic;

#ifndef hal_flag_ARMv8_1
#error "Atomic operations require ARMv8.1 or later"
#else

#define HAL_LIB_ATOMIC_ADD

__always_inline__ void hal_Atomic_add(hal_Atomic *atomic, i64 value) {
	i64 tmp;
	__asm__ volatile (
		"ldadd %x1, %x0, [%2]\n"
		: "=&r"(tmp)
		: "r"(value), "r"(&atomic->value)
		: "memory"
	);
}

#define HAL_LIB_ATOMIC_SUB

__always_inline__ void hal_Atomic_sub(hal_Atomic *atomic, i64 value) {
	i64 tmp;
	__asm__ volatile (
		"ldadd %x1, %x0, [%2]\n"
		: "=&r"(tmp)
		: "r"(-value), "r"(&atomic->value)
		: "memory"
	);
}

#define HAL_LIB_ATOMIC_INC
__always_inline__ void hal_Atomic_inc(hal_Atomic *atomic) {
	i64 tmp;
	 __asm__ volatile (
		"ldadd xzr, %x0, [%1]	\n\t"
		"add %x0, %x0, #1	\n\t"
		"stadd %x0, [%1]	\n\t"
		: "=&r"(tmp)
		: "r"(&atomic->value)
		: "memory"
	);
}

#define HAL_LIB_ATOMIC_DEC
__always_inline__ void hal_Atomic_dec(hal_Atomic *atomic) {
	i64 tmp;
	__asm__ volatile (
		"ldadd xzr, %x0, [%1]	\n\t"
		"sub %x0, %x0, #1	\n\t"
		"stadd %x0, [%1]	\n\t"
		: "=&r"(tmp)
		: "r"(&atomic->value)
		: "memory"
	);
}

#define HAL_LIB_ATOMIC_BTS

#endif
#endif