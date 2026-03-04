#ifndef __CPU_DESC_H__
#define __CPU_DESC_H__

#include <lib/dtypes.h>
#include <cpu/define.h>

extern u8 cpu_symbol_percpu;
extern u8 cpu_symbol_epercpu;

extern u32 cpu_num;
extern u32 cpu_bspIdx;

extern void **cpu_bsAddr;

cpu_declarevar(u64, cpu_bsAddr);
cpu_declarevar(u64, cpu_id);
cpu_declarevar(u64, cpu_state);
cpu_declarevar(u64, cpu_flag);

#endif