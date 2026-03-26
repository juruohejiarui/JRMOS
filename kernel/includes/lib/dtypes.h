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

#ifndef __used__
#define __used__ __attribute__((used))
#endif 

#ifndef __noinline__
#define __noinline__ __used__ __attribute__ ((noinline))
#endif

#ifndef __likely__
#define __likely__(x) __builtin_expect(!!(x), 1)
#endif

#ifndef __unlikely__
#define __unlikely__(x) __builtin_expect(!!(x), 0)
#endif

#ifndef offsetof
#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)
#endif

#define NULL ((void *)0)

#define res_SUCC	0x0
#define res_FAIL	0x1
#define res_BUSY 	0x2
#define res_DAMAGED	0x4

#define Page_4KShift	12
#define Page_2MShift	21
#define Page_1GShift	30

#define Page_4KSize		(1ull << Page_4KShift)
#define Page_2MSize		(1ull << Page_2MShift)
#define Page_1GSize		(1ull << Page_1GShift)

#define Page_4KMask		(~(Page_4KSize - 1))
#define Page_2MMask		(~(Page_2MSize - 1))
#define Page_1GMask		(~(Page_1GSize - 1))

#define applyOffset(base, off, type) ({ \
    register volatile u64 _t = (u64)base; \
    (type)(_t + (off)); \
})

#define container(memberAddr, tp, memberIden) applyOffset((memberAddr), -offsetof(tp, memberIden), tp *)

#define mwrite(ptr, val) (*(volatile __typeof__(*(ptr)) *)(ptr) = (val))
#define mread(ptr) (*(volatile __typeof__(*(ptr)) *)(ptr))

#define __optimize__ __attribute__((optimize("O1")))

#ifndef __always_inline__
// inline function must be static function
#define __always_inline__ static inline __optimize__ __attribute__ ((always_inline))
#endif

#endif
