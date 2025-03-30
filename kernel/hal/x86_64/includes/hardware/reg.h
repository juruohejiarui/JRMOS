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
#endif