#ifndef __CPU_DESC_H__
#define __CPU_DESC_H__

#include <hal/cpu/desc.h>
#include <interrupt/desc.h>

#define cpu_mxNum hal_cpu_mxNum

#define cpu_Desc_state_Active   0x1
#define cpu_Desc_state_Trapped  0x2
#define cpu_Desc_state_Pause    0x3

typedef struct cpu_Desc {
    u64 state;
    u64 intrMsk[64];
    u32 intrUsage, intrFree;
    intr_Info *intrInfo[0x40];
    hal_cpu_Desc hal;
} __attribute__ ((packed)) cpu_Desc;

extern u32 cpu_num;
extern u32 cpu_desc[cpu_mxNum];

#endif