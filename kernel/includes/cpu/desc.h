#ifndef __CPU_DESC_H__
#define __CPU_DESC_H__

#include <hal/cpu/desc.h>
#include <interrupt/desc.h>
#include <softirq/desc.h>
#include <lib/spinlock.h>

#define cpu_mxNum hal_cpu_mxNum

#define cpu_Desc_state_Active   0x1
#define cpu_Desc_state_Trapped  0x2
#define cpu_Desc_state_Pause    0x3

// when you need to modify the structure of this descriptor
// please update the corresponding constant in cpu/asmdesc.h
typedef struct cpu_Desc {
    u64 state;
    SpinLock intrMskLck;
    u64 intrMsk[4];
    u32 intrUsage, intrFree;
    u64 sirqMsk;
    u64 sirqFlag;
    u32 sirqUsage, sirqFree;
    softirq_Desc sirq[64];
    hal_cpu_Desc hal;
} __attribute__ ((packed)) cpu_Desc;

#define cpu_irq_Schedule    0x0
#define cpu_irq_Pause       0x1
#define cpu_irq_Execute     0x2

extern u32 cpu_num;
extern u32 cpu_bspIdx;
extern cpu_Desc cpu_desc[cpu_mxNum];

#endif