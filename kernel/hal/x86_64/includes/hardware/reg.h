#ifndef __HAL_HARDWARE_REG_H__
#define __HAL_HARDWARE_REG_H__

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
#endif