#ifndef __CPU_DESC_H__
#define __CPU_DESC_H__

#include <hal/cpu/desc.h>
#include <interrupt/desc.h>
#include <softirq/desc.h>
#include <lib/spinlock.h>
#include <lib/rbtree.h>

#define cpu_mxNum hal_cpu_mxNum

#define cpu_Desc_state_Active   0x1
#define cpu_Desc_state_Trapped  0x2
#define cpu_Desc_state_Pause    0x3

#define cpu_intr_Schedule   hal_cpu_intr_Schedule
#define cpu_intr_Pause      hal_cpu_intr_Pause
#define cpu_intr_Execute    hal_cpu_intr_Execute

// when you need to modify the structure of this descriptor
// please update the corresponding constant in cpu/asmdesc.h
typedef struct cpu_Desc {
    volatile u64 state;
    SpinLock intrMskLck;
    u64 intrMsk[4];
    u64 sirqMsk;
    u64 sirqFlag;
    u16 intrUsage, intrFree;
    u16 sirqUsage, sirqFree;
    softirq_Desc sirq[64];
    RBTree *tskTree;
    hal_cpu_Desc hal;
} __attribute__ ((packed)) cpu_Desc;

extern u32 cpu_num;
extern u32 cpu_bspIdx;
extern cpu_Desc cpu_desc[cpu_mxNum];

#endif