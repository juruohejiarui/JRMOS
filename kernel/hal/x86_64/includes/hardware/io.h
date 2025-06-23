#ifndef __HAL_LIB_hal_hw_io_H__
#define __HAL_LIB_hal_hw_io_H__

#include <lib/dtypes.h>

__always_inline__ void hal_hw_io_out8(u16 port, u8 data) {
    __asm__ volatile (
        "outb %0, %%dx   \n\t"
        "mfence          \n\t"
        :
        : "a"(data), "d"(port)
        : "memory"
    );
}

__always_inline__ void hal_hw_io_out16(u16 port, u16 data) {
    __asm__ volatile (
        "outw %0, %%dx   \n\t"
        "mfence          \n\t"
        :
        : "a"(data), "d"(port)
        : "memory"
    );
}

__always_inline__ void hal_hw_io_out32(u16 port, u32 data) {
    __asm__ volatile (
        "outl %0, %%dx   \n\t"
        "mfence          \n\t"
        :
        : "a"(data), "d"(port)
        : "memory"
    );
}

__always_inline__ u8 hal_hw_io_in8(u16 port) {
    u8 ret = 0;
    __asm__ volatile (
        "inb %%dx, %0   \n\t"
        "mfence         \n\t"
        : "=a"(ret)
        : "d"(port)
        : "memory"
    );
    return ret;
}

__always_inline__ u16 hal_hw_io_in16(u16 port) {
    u16 ret = 0;
    __asm__ volatile (
        "inw %%dx, %0   \n\t"
        "mfence         \n\t"
        : "=a"(ret)
        : "d"(port)
        : "memory"
    );
    return ret;
}

__always_inline__ u32 hal_hw_io_in32(u16 port) {
    u32 ret = 0;
    __asm__ volatile (
        "inl %%dx, %0   \n\t"
        "mfence         \n\t"
        : "=a"(ret)
        : "d"(port)
        : "memory"
    );
    return ret;
}

#define hal_hw_write(addr, val, tp) \
    __asm__ volatile ( \
        "mov %1, (%0)   \n\t" \
        "mfence         \n\t" \
        : \
        : "r"(addr), "r"((tp)(val)) \
        : "memory" \
    );

#define hal_hw_read(addr, tp) ({\
    tp val; \
    __asm__ volatile ( \
        "mov (%0), %1   \n\t" \
        : \
        : "r"(addr), "r"(val) \
        : "memory" \
    ); \
    val; \
})
#endif