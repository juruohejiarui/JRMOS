#ifndef __HAL_CPU_API_H__
#define __HAL_CPU_API_H__

#include <cpu/desc.h>
#include <hal/hardware/apic.h>

#define hal_cpu_getvar(name) ({ \
    __typeof__(cpu_var(name)) val; \
    __asm__ volatile ( \
        "mov %%gs:(%1), %0" \
        : "=r" (val) \
        : "r" (cpu_varOffset(name)) \
        : "memory" \
    ); \
    val; \
})

#define hal_cpu_getCpuVar(idx, name) ({ \
    __typeof__(cpu_var(name)) val; \
    __asm__ volatile ( \
        "mov (%1), %0" \
        : "=r" (val) \
        : "r"((u64)cpu_bsAddr[idx] + cpu_varOffset(name)) \
        : "memory" \
    ); \
    val; \
})

#define hal_cpu_setvar(name, val) ({ \
    __asm__ volatile ( \
        "mov %1, %%gs:(%0)" \
        : \
        : "r" (cpu_varOffset(name)), "r" (val) \
        : "memory" \
    ); \
})

#define hal_cpu_setCpuVar(idx, name, val) ({ \
    __asm__ volatile ( \
        "mov %1, (%0)" \
        : \
        : "r"((u64)cpu_bsAddr[idx] + cpu_varOffset(name)), "r" (val) \
        : "memory" \
    ); \
})

#define hal_cpu_ptr(name) \
    ((__typeof__(cpu_var(name))*) \
        ((u64)cpu_getvar(cpu_bsAddr) + cpu_varOffset(name)))

#define hal_cpu_cpuPtr(idx, name) \
    ((__typeof__(cpu_var(name))*) \
        ((u64)cpu_bsAddr[idx] + cpu_varOffset(name)))


int hal_cpu_init();

int hal_cpu_enableAP();

int hal_cpu_initIntr();

__always_inline__ void hal_cpu_sendIntr_allExcluSelf(u64 irq){
	hal_hw_apic_writeIcr32(hal_hw_apic_makeIcr32(irq, hal_hw_apic_DestShorthand_AllExcludingSelf));
}

__always_inline__ void hal_cpu_sendIntr_self(u64 irq){
	hal_hw_apic_writeIcr32(hal_hw_apic_makeIcr32(irq, hal_hw_apic_DestShorthand_Self));
}

__always_inline__ void hal_cpu_sendIntr_all(u64 irq){
	hal_hw_apic_writeIcr(hal_hw_apic_makeIcr32(irq, hal_hw_apic_DestShorthand_AllIncludingSelf));
}

void hal_cpu_sendIntr(u64 irq, u32 cpuId);

// check current processor and set flags
void hal_cpu_chk();

#endif