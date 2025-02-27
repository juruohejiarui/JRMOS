#ifndef __LIB_ATOMIC_H__
#define __LIB_ATOMIC_H__

#include <hal/lib/atomic.h>

typedef hal_Atomic Atomic;

#ifdef HAL_LIB_ATOMIC_ADD
#define Atomic_add hal_Atomic_add
#else
#error no definition of Atomic_add() for this arch!
#endif

#ifdef HAL_LIB_ATOMIC_SUB
#define Atomic_sub hal_Atomic_sub
#else
#error no definition of Atomic_add() for this arch!
#endif

#ifdef HAL_LIB_ATOMIC_INC
#define Atomic_inc hal_Atomic_inc
#else
#error no definition of Atomic_INC() for this arch!
#endif

#ifdef HAL_LIB_ATOMIC_DEC
#define Atomic_dec hal_Atomic_dec
#else
#error no definition of Atomic_dec() for this arch!
#endif
#endif