#include <hal/hardware/cpu.h>

void hal_hw_cpu_cpuid(u32 mop, u32 sop, u32 *a, u32 *b, u32 *c, u32 *d) {
	__asm__ volatile (
        "cpuid \n\t"
        : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
        : "0"(mop), "2"(sop)
    );
}