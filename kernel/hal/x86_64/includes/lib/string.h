#ifndef __HAL_LIB_STRING_H__
#define __HAL_LIB_STRING_H__

#include <lib/dtypes.h>

#define HAL_MEMSET
#define HAL_MEMCPY
#define HAL_MEMCMP
#define HAL_STRLEN
#define HAL_STRCMP
#define HAL_STRNCMP

__always_inline__ void *memset(void *addr, u8 dt, i64 size) {
	int d0, d1;
	u64 tmp = dt * 0x0101010101010101UL;
	__asm__ volatile (
		"cld					\n\t"
		"rep					\n\t"
		"stosq				  	\n\t"
		"testb $4, %b3		  	\n\t"
		"je 1f				  	\n\t"
		"stosl				  	\n\t"
		"1:\ttestb $2, %b3	  	\n\t"
		"je 2f				  	\n\t"
		"stosw				  	\n\t"
		"2:\ttestb $1, %b3	  	\n\t"
		"je 3f				  	\n\t"
		"stosb				  	\n\t"
		"3:					 	\n\t"
		: "=&c"(d0), "=&D"(d1)
		: "a"(tmp), "q"(size), "0"(size >> 3), "1"(addr)
		: "memory"
	);
	return addr;
}

__always_inline__ void *memcpy(void *src, void *dst, i64 num) {
	int d0, d1, d2;
	__asm__ volatile(
		"cld				\n\t"
		"rep				\n\t"
		"movsq				\n\t"
		"testb	$4, %b4		\n\t"
		"je	1f				\n\t"
		"movsl				\n\t"
		"1:\ttestb	$2, %b4	\n\t"
		"je	2f				\n\t"
		"movsw				\n\t"
		"2:\ttestb	$1,%b4	\n\t"
		"je	3f				\n\t"
		"movsb				\n\t"
		"3:					\n\t"
		: "=&c"(d0), "=&D"(d1), "=&S"(d2)
		: "0"(num / 8), "q"(num), "1"(dst), "2"(src)
		: "memory");
	return dst;
}

__always_inline__ int memcmp(void *fir, void *sec, i64 size) {
	register int res;
	__asm__ volatile (
		"cld			\n\t"
		"repe		   	\n\t"
		"cmpsb		  	\n\t"
		"je 1f		  	\n\t"
		"movl $1, %%eax \n\t"
		"jl 1f		  	\n\t"
		"negl %%eax	 	\n\t"
		"1:			 	\n\t"
		: "=a"(res)
		: "0"(0), "D"(fir), "S"(sec), "c"(size)
		:
	);
	return res;
}

__always_inline__ i64 strlen(u8 *str) {
	register i64 res;
	__asm__ volatile (
		"cld		\n\t"
		"repne	  	\n\t"
		"scasb	  	\n\t"
		"notq %0	\n\t"
		"decq %0	\n\t"
		: "=c"(res)
		: "D"(str), "a"(0), "0"(0xffffffffffffffff)
	);
	return res;
}


__always_inline__ int strcmp(char *a, char *b) {
	register int _res;
	__asm__ volatile (
		"cld				\n\t"
		"1: 				\n\t"
		"lodsb				\n\t"
		"scasb				\n\t"
		"jne 2f				\n\t"
		"testb %%al, %%al	\n\t"
		"jne 1b				\n\t"
		"xorl %%eax, %%eax	\n\t"
		"jmp 3f				\n\t"
		"2:					\n\t"
		"movl $1, %%eax		\n\t"
		"jl 3f				\n\t"
		"negl %%eax			\n\t"
		"3:					\n\t"
		: "=a"(_res)
		: "D"(a), "S"(b)
		:
	);
	return _res;
}

__always_inline__ int strncmp(char *a, char *b, i64 size) {
	register int _res;
	__asm__ volatile (
		"cld 				\n\t"
		"1: 				\n\t"
		"decq %3			\n\t"
		"js 2f				\n\t"
		"lodsb				\n\t"
		"scasb				\n\t"
		"jne 3f				\n\t"
		"testb %%al, %%al	\n\t"
		"jne 1b				\n\t"
		"2:					\n\t"
		"xorl %%eax, %%eax	\n\t"
		"jmp 4f				\n\t"
		"3:					\n\t"
		"movl $1, %%eax		\n\t"
		"jl 4f				\n\t"
		"negl %%eax			\n\t"
		"4:					\n\t"
		: "=a"(_res)
		: "D"(a), "S"(b), "c"(size)
		:
	);
	return _res;
}

#endif