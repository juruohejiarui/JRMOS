#ifndef __HAL_LIB_CRC32_H__
#define __HAL_LIB_CRC32_H__

#include <lib/dtypes.h>
#define HAL_LIB_CRC32

static __always_inline__ u32 hal_crc32_u8(u32 crc, u8 dt) {
	__asm__ volatile (
		"crc32b %1, %0	\n\t"
		: "+r"(crc)
		: "r"(dt)
	);
	return crc;
}

static __always_inline__ u32 hal_crc32_u32(u32 crc, u8 dt) {
	__asm__ volatile (
		"crc32l	%1, %0	\n\t"
		: "+r"(crc)
		: "r"(dt)
	);
	return crc;
}
#endif