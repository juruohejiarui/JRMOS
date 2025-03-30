#ifndef __HAL_CPU_API_H__
#define __HAL_CPU_API_H__

#include <cpu/desc.h>

#define hal_cpu_getvar(name) ({ \
    __typeof__(((cpu_Desc *)NULL)->name) val; \
    __asm__ volatile ( \
        "mov %%gs:(%1), %0" \
        : "=r" (val) \
        : "r" ((u64)&(((cpu_Desc *)NULL)->name)) \
    ); \
	val; \
})


#define hal_cpu_setvar(name, val) ({ \
    __asm__ volatile ( \
        "mov %0, %%gs:(%1)" \
        : \
        : "r" (val), "r" ((u64)&(((cpu_Desc *)NULL)->name)) \
    ); \
})

int hal_cpu_init();

int hal_cpu_enableAP();

int hal_cpu_initIntr();

void hal_cpu_sendIntr_all(u64 irq);

void hal_cpu_sendIntr_allExcluSelf(u64 irq);

void hal_cpu_sendIntr_self(u64 irq);

void hal_cpu_sendIntr(u64 irq, u32 cpuId);

#endif