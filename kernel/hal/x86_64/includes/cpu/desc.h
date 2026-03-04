#ifndef __HAL_CPU_DESC_H__
#define __HAL_CPU_DESC_H__

#include <lib/dtypes.h>
#include <cpu/desc.h>

#define hal_cpu_mxNum	128

#define hal_cpu_intr_Schedule   0x80
#define hal_cpu_intr_Pause      0x81
#define hal_cpu_intr_Execute    0x82

#define hal_cpu_flags_sse42     (1ul << 0)
#define hal_cpu_flags_xsave     (1ul << 1)
#define hal_cpu_flags_osxsave   (1ul << 2)
#define hal_cpu_flags_avx       (1ul << 3)
#define hal_cpu_flags_avx512    (1ul << 4)

extern u32 hal_cpu_bspApicId;

extern u8 hal_cpu_apBootEntry[];
extern u8 hal_cpu_apBootEnd[];

cpu_declarevar(u32, hal_cpu_x2apic);
cpu_declarevar(u32, hal_cpu_apic);
cpu_declarevar(u8 *, hal_cpu_initStk);
cpu_declarevar(u64, hal_cpu_flags);


#endif