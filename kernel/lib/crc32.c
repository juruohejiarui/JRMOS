#include <lib/crc32.h>

u32 crc32(u8 *dt, u64 size) {
	u32 crc = crc32_init();
	for (u64 i = 0; i < size; i += 4) crc = crc32_u32(crc, *(u32 *)(dt + i));
	for (int i = size & ~0x7; i < size; i++) crc = crc32_u8(crc, dt[i]);
	return crc32_end(crc);
}

u32 crc32_ieee802(u8 *dt, u64 size) {
	u32 crc = 0xffffffffu; // Initial value for CRC-32
	for (u64 i = 0; i < size; i++) {
		crc ^= (u32)dt[i];
		for (int bit = 0; bit < 8; bit++) {
			if (crc & 0x1) crc = ((crc >> 1) ^ 0xedb88320u); // Polynomial for CRC-32
			else crc >>= 1;
		}
	}
	return ~crc;
}

#ifndef HAL_LIB_CRC32
static u32 crc32_tbl[256] = {0};

void crc32_initTbl() {
	for (u32 i = 0; i < 256; i++) {
		u32 crc = i;
		for (int j = 0; j < 8; j++) {
			if (crc & 1u) crc = (crc >> 1) ^ crc32_poly;
			else crc >>= 1;
		}
		crc32_tbl[i] = crc;
	}
}

u32 crc32_u8(u32 crc, u8 dt) {
	return crc32_tbl[(crc ^ dt) & 0xffu] ^ (crc >> 8);
}

u32 crc32_u32(u32 crc, u32 dt) {
	crc = crc32_u8(crc, dt & 0xffu);
	crc = crc32_u8(crc, (dt >> 8) & 0xff);
	crc = crc32_u8(crc, (dt >> 16) & 0xff);
	return crc32_u8(crc, (dt >> 24) & 0xff);
}
#endif