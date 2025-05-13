#ifndef __HAL_HARDWARE_REG_H__
#define __HAL_HARDWARE_REG_H__

#include <lib/dtypes.h>

#define hal_hw_setCR(id, vl) do { \
	u64 vr = (vl); \
	__asm__ volatile ( \
		"movq %0, %%rax	\n\t"\
		"movq %%rax, %%cr"#id"\n\t" \
		: "=m"(vr) \
		: \
		: "rax", "memory"); \
} while (0)

#define hal_hw_getCR(id)  ({ \
	u64 vl; \
	__asm__ volatile ( \
		"movq %%cr"#id", %%rax	\n\t"\
		"movq %%rax, %0" \
		: "=m"(vl) \
		: \
		: "rax", "memory"); \
	vl; \
})

#define hal_hw_mfence() __asm__ volatile ("mfence 	\n\t" : : : "memory")

static __always_inline__ u64 hal_hw_readMsr(u64 msrAddr) {
    u32 data1, data2;
    __asm__ volatile (
        "rdmsr \n\t"
        : "=d"(data1), "=a"(data2)
        : "c"(msrAddr)
        : "memory"
    );
    return (((u64) data1) << 32) | data2;
}

static __always_inline__ void hal_hw_writeMsr(u64 msrAddr, u64 data) { 
    __asm__ volatile (
        "wrmsr \n\t"
        :
        : "c"(msrAddr), "A"(data & 0xFFFFFFFFul), "d"((data >> 32) & 0xFFFFFFFFul)
        : "memory"
    );
}

#define hal_hw_hlt() __asm__ volatile ("hlt	\n\t" : : : "memory");

#define hal_msr_IA32_EFER			0xC0000080

#define hal_msr_IA32_FS_BASE		0xC0000100
#define hal_msr_IA32_GS_BASE		0xC0000101
#define hal_msr_IA32_KERNEL_GS_BASE 0xC0000102

static __always_inline__ u64 hal_read64(u64 addr) {
	u64 val;
	__asm__ volatile (
		"movq (%1), %0	\n\t"
		: "=b"(val)
		: "a"(addr)
		: "memory"
	);
	return val;
}

static __always_inline__ u32 hal_read32(u64 addr) {
	u32 val;
	__asm__ volatile (
		"movl (%1), %0	\n\t"
		: "=b"(val)
		: "a"(addr)
		: "memory"
	);
	return val;
}

static __always_inline__ u16 hal_read16(u64 addr) { return (hal_read32(addr & ~0x3ul) >> ((addr & 0x3) << 3)) & 0xffff; }

static __always_inline__ u8 hal_read8(u64 addr) { return (hal_read32(addr & ~0x3ul) >> ((addr & 0x3) << 3)) & 0xff; }

static __always_inline__ void hal_write64(u64 addr, u64 val) {
	__asm__ volatile (
		"movq %%rax, (%%rbx)	\n\t"
		"mfence					\n\t"
		: 
		: "a"(val), "b"(addr)
		: "memory"
	);
}

static __always_inline__ void hal_write32(u64 addr, u32 val) {
	__asm__ volatile (
		"movl %%eax, (%%rbx)	\n\t"
		"mfence					\n\t"
		: 
		: "a"(val), "b"(addr)
		: "memory"
	);
}

static __always_inline__ void hal_write16(u64 addr, u16 val) {
	u32 prev = hal_read32(addr & ~0x3ul), mask = 0xffff << ((addr & 0x3) << 3);
	hal_write32(addr & ~0x3ul, (prev & ~mask) | ((u32)val << ((addr & 0x3) << 3)));
}

static __always_inline__ void hal_write8(u64 addr, u8 val) {
	u32 prev = hal_read32(addr & ~0x3ul), mask = 0xff << ((addr & 0x3) << 3);
	hal_write32(addr & ~0x3ul, (prev & ~mask) | ((u32)val << ((addr & 0x3) << 3)));
}
#endif