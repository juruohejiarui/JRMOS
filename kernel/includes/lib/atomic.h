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
#error no definition of Atomic_inc() for this arch!
#endif

#ifdef HAL_LIB_ATOMIC_DEC
#define Atomic_dec hal_Atomic_dec
#else
#error no definition of Atomic_dec() for this arch!
#endif

#ifdef HAL_LIB_ATOMIC_BTS
#define Atomic_bts hal_Atomic_bts
#else
#error no definition of Atomic_bts() for this arch!
#endif

#ifdef HAL_LIB_ATOMIC_BTC
#define Atomic_btc hal_Atomic_btc
#else
#error no definition of Atomic_btc() for this arch!
#endif

#ifdef HAL_LIB_ATOMIC_BTR
#define Atomic_btr hal_Atomic_btr
#else
#error no definition of Atomic_btr() for this arch!
#endif
#endif