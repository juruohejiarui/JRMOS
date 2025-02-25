#ifndef __LIB_BIT_H__
#define __LIB_BIT_H__

#include <hal/lib/bit.h>

#ifndef HAL_BIT_FFS
#error no definition of bit_ffs32 and bit_ffs64 for this arch!
#else
#define bit_ffs32 hal_bit_ffs32
#define bit_ffs64 hal_bit_ffs64
#endif

#ifndef HAL_BIT_SET0
#error no definition of bit_set0 for this arch!
#else
#define bit_set0 hal_bit_set0
#endif

#ifndef HAL_BIT_SET1
#error no definition of bit_set1 for this arch!
#else
#define bit_set1 hal_bit_set1
#endif

#ifndef HAL_BIT_REV
#error no definition of bit_rev for this arch!
#else
#define bit_rev hal_bit_rev
#endif

#endif