#ifndef __HAL_LIB_BIT_H__
#define __HAL_LIB_BIT_H__

#include <lib/dtypes.h>

#define HAL_BIT_FFS
__always_inline__ u8 hal_bit_ffs64(u64 val) {
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
__always_inline__ u8 hal_bit_ffs32(u32 val) {
	u32 n = 1;
	__asm__ volatile (
		"bsfl %1, %%eax				\n\t"
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
__always_inline__ void hal_bit_set0(u64 *addr, u64 index) {
	__asm__ volatile (
		"btrq %1, %0		\n\t"
		: "+m"(*addr)
		: "r"(index)
		: "memory");
}

__always_inline__ void hal_bit_set0_32(u32 *addr, u32 index) {
	__asm__ volatile (
		"btrl %1, %0		\n\t"
		: "+m"(*addr)
		: "r"(index)
		: "memory");
}

__always_inline__ void hal_bit_set0_16(u16 *addr, u16 index) { 
	__asm__ volatile (
		"btrw %1, %0		\n\t"
		: "+m"(*addr)
		: "r"(index)
		: "memory");
}

#define HAL_BIT_SET1
__always_inline__ void hal_bit_set1(u64 *addr, u64 index) { 
	__asm__ volatile (
		"btsq %1, %0		\n\t"
		: "+m"(*addr)
		: "r"(index)
		: "memory");
}

__always_inline__ void hal_bit_set1_32(u32 *addr, u32 index) { 
	__asm__ volatile (
		"btsl %1, %0		\n\t"
		: "+m"(*addr)
		: "r"(index)
		: "memory");
}

__always_inline__ void hal_bit_set1_16(u16 *addr, u16 index) { 
	__asm__ volatile (
		"btsw %1, %0		\n\t"
		: "+m"(*addr)
		: "r"(index)
		: "memory");
}

#define HAL_BIT_REV
__always_inline__ void hal_bit_rev(u64 *addr, u64 index) {
    __asm__ volatile (
        "btcq %1, %0    \n\t"
        : "+m"(*addr)
        : "r"(index)
        : "memory"
    );
}

__always_inline__ void hal_bit_rev_32(u32 *addr, u32 index) {
    __asm__ volatile (
        "btcl %1, %0    \n\t"
        : "+m"(*addr)
        : "r"(index)
        : "memory"
    );
}

__always_inline__ void hal_bit_rev_16(u16 *addr, u16 index) {
    __asm__ volatile (
        "btcw %1, %0    \n\t"
        : "+m"(*addr)
        : "r"(index)
        : "memory"
    );
}
#endif