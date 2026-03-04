#ifndef __CPU_API_H__
#define __CPU_API_H__

#include <cpu/desc.h>
#include <hal/cpu/api.h>

#define cpu_sendIntr_all hal_cpu_sendIntr_all

#define cpu_sendIntr_allExcluSelf hal_cpu_sendIntr_allExcluSelf

#define cpu_sendIntr_self hal_cpu_sendIntr_self

#define cpu_sendIntr hal_cpu_sendIntr

#define cpu_getvar hal_cpu_getvar

#define cpu_setvar hal_cpu_setvar

#define cpu_ptr hal_cpu_ptr

#define cpu_getCpuVar hal_cpu_getCpuVar

#define cpu_setCpuVar hal_cpu_setCpuVar

#define cpu_cpuPtr hal_cpu_cpuPtr

int cpu_initVar();

#endif