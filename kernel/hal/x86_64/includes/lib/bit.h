#ifndef __HAL_LIB_BIT_H__
#define __HAL_LIB_BIT_H__

#include <lib/dtypes.h>

#define HAL_BIT_FFS
static __always_inline__ u8 hal_bit_ffs64(u64 val) {
	u32 n = 1;
	__asm__ volatile (
		"bsfq %1, %%rax				\n\t"
		"movl $0xffffffff, %%edx	\n\t"
		"cmove %%edx, %%eax			\n\t"
		"addl $1, %%eax				\n\t"
		"movl %%eax, %0				\n\t"
		: "=m"(n)
		: "m"(val)
		: "memory", "rax", "rdx"
	);
	return n;
}
static __always_inline__ u8 hal_bit_ffs32(u32 val) {
	u32 n = 1;
	__asm__ volatile (
		"bsf %1, %%eax				\n\t"
		"movl $0xffffffff, %%edx	\n\t"
		"cmove %%edx, %%eax			\n\t"
		"addl $1, %%eax				\n\t"
		"movl %%eax, %0				\n\t"
		: "=m"(n)
		: "m"(val)
		: "memory", "rax", "rdx"
	);
	return n;
} 

#define HAL_BIT_SET0
static __always_inline__ void hal_bit_set0(u64 *addr, u64 index) {
	__asm__ volatile (
		"btrq %1, %0		\n\t"
		: "+m"(*addr)
		: "r"(index)
		: "memory");
}

static __always_inline__ void hal_bit_set0_32(u32 *addr, u64 index) {
	__asm__ volatile (
		"btr %1, %0		\n\t"
		: "+m"(*addr)
		: "r"(index)
		: "memory");
}

static __always_inline__ void hal_bit_set0_16(u16 *addr, u64 index) { 
	__asm__ volatile (
		"btr %1, %0		\n\t"
		: "+m"(*addr)
		: "r"(index)
		: "memory");
}

#define HAL_BIT_SET1
static __always_inline__ void hal_bit_set1(u64 *addr, u64 index) { 
	__asm__ volatile (
		"btsq %1, %0		\n\t"
		: "+m"(*addr)
		: "r"(index)
		: "memory");
}

static __always_inline__ void hal_bit_set1_32(u32 *addr, u64 index) { 
	__asm__ volatile (
		"bts %1, %0		\n\t"
		: "+m"(*addr)
		: "r"(index)
		: "memory");
}

static __always_inline__ void hal_bit_set1_16(u16 *addr, u64 index) { 
	__asm__ volatile (
		"bts %1, %0		\n\t"
		: "+m"(*addr)
		: "r"(index)
		: "memory");
}

#define HAL_BIT_REV
static __always_inline__ void hal_bit_rev(u64 *addr, u64 index) {
    __asm__ volatile (
        "btcq %1, %0    \n\t"
        : "+m"(*addr)
        : "r"(index)
        : "memory"
    );
}

static __always_inline__ void hal_bit_rev_32(u32 *addr, u64 index) {
    __asm__ volatile (
        "btc %1, %0    \n\t"
        : "+m"(*addr)
        : "r"(index)
        : "memory"
    );
}

static __always_inline__ void hal_bit_rev_16(u16 *addr, u64 index) {
    __asm__ volatile (
        "btc %1, %0    \n\t"
        : "+m"(*addr)
        : "r"(index)
        : "memory"
    );
}
#endif