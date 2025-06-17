#include <lib/hash.h>

void hash_init() {
#ifndef HAL_LIB_CRC32
	crc32_initTbl();
#endif
}