#ifndef __LIB_STRING_H__
#define __LIB_STRING_H__

#include <hal/lib/string.h>

#ifndef HAL_MEMSET
__always_inline__ void *memset(void *addr, u8 dt, i64 size) {
	i64 i;
	u64 dt64 = dt;
	void *dest = addr;
	dt64 |= (dt64 << 8);
	dt64 |= (dt64 << 16);
	dt64 |= (dt64 << 32);

	while (((u64)addr % 8) && size) {
		*(u8 *)addr = dt;
		size--, addr++;
	}
	for (i = 0; i < size / 8; i++) ((u64 *)addr)[i] = dt64;
	addr += size & ~0x7ul;
	for (i = 0; i < size & 0x7; i++) ((u8 *)addr)[i] = dt;
	return dest;
}
#endif

#ifndef HAL_MEMCPY
__always_inline__ void *memcpy(void *src, void *dst, i64 size) {
	void *addr2 = dst;
	i64 i;
	while (((u64)src % 8) && ((u64)addr2 % 8) && size) {
		*(u8 *)addr2 = *(u8 *)src;
		size--, src++, addr2++;
	}
	for (i = 0; i < size / 8; i++) ((u64 *)addr2)[i] = ((u64 *)src)[i];
	src += size & ~0x7ul, addr2 += size & ~0x7ul;
	for (i = 0; i < size & 0x7; i++) ((u8 *)addr2)[i] = ((u8 *)src)[i];
	return dst;
}
#endif

#ifndef HAL_MEMCMP
__always_inline__ int memcmp(void *fir, void *sec, i64 size) {
	for (i64 i = 0; i < size; i++) {
		if (*(fir + i) != *(sec + i)) return *(fir + i) < *(sec + i) ? 1 : -1; 
	}
	return 0;
}
#endif

#ifndef HAL_STRLEN
__always_inline__ u64 strlen(u8 *str) {
	i64 res;
	for (res = 0; str[res] != '\0'; res++) ;
	return res;
}
__always_inline__ u64 strlen16(u16 *str) {
	i64 res;
	for (res = 0; str[res] != 0; res++) ;
	return res; 
}
#endif

__always_inline__ int isLetter(u8 ch) {
	return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z');
}

__always_inline__ int isLetter16(u16 ch) {
	register u8 h = (ch >> 8), l = (ch & 0xff);
	return (h == 0) && isLetter(l); 
}

__always_inline__ u8 toUpper(u8 ch) {
	return !isLetter(ch) || ('A' <= ch && ch <= 'Z') ? ch : ch - 'a' + 'A';
}
__always_inline__ u8 toLower(u8 ch) {
	return !isLetter(ch) || ('a' <= ch && ch <= 'z') ? ch : ch - 'A' + 'a';
}

__always_inline__ u64 toStr16(u8 *src, u16 *dst) {
	u64 len = 0;
	for (; *src != '\0'; src++, dst++, len++) {
		*dst = *src;
	}
	*dst = '\0';
	return len;
}
#endif