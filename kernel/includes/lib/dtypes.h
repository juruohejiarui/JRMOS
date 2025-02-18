#ifndef __LIB_DTYPES_H__
#define __LIB_DTYPES_H__

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long i64;

typedef float f32;
typedef double f64;

#define __always_inline__  inline __attribute__ ((always_inline))
#define __noinline__ __attribute__ ((noinline))

#define NULL ((void *)0)

#define res_SUCC	0x0
#define res_FAIL	0x1

#define Page_4KShift	12
#define Page_2MShift	21
#define Page_1GShift	30

#define Page_4KSize		(1ul << Page_4KShift)
#define Page_2MSize		(1ul << Page_2MShift)
#define Page_1GSize		(1ul << Page_1GShift)

#define Page_4KMask		(Page_4KSize - 1)
#define Page_2MMask		(Page_2MSize - 1)
#define Page_1GMask		(Page_1GSize - 1)


#endif
