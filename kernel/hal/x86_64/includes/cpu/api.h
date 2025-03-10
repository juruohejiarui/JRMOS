#ifndef __HAL_CPU_API_H__
#define __HAL_CPU_API_H__

#include <cpu/desc.h>

int hal_cpu_init();

int hal_cpu_initIntr();

void hal_cpu_sendIntr_all(u64 irq);

void hal_cpu_sendIntr_allExcluSelf(u64 irq);

void hal_cpu_sendIntr_self(u64 irq);

void hal_cpu_sendIntr(u64 irq, u32 cpuId);

#endif