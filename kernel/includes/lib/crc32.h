#ifndef __LIB_CRC32_H__
#define __LIB_CRC32_H__

#include <hal/lib/crc32.h>

__always_inline__ u32 crc32_init() { return 0xffffffff; }
__always_inline__ u32 crc32_end(u32 crc) { return ~crc; }

u32 crc32_ieee802(u8 *dt, u64 size);

#ifdef HAL_LIB_CRC32

#define crc32_u8 hal_crc32_u8
#define crc32_u32 hal_crc32_u32

#else
#warning no definition of "crc()" for this arch

#define crc32_poly	0xedb88320u

void crc32_initTbl();

u32 crc32_u8(u32 crc, u8 dt);
u32 crc32_u32(u32 crc, u32 dt);
#endif

#endif