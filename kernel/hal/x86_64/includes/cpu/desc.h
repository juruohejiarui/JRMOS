#ifndef __HAL_CPU_DESC_H__
#define __HAL_CPU_DESC_H__

#include <lib/dtypes.h>
#include <hal/interrupt/desc.h>
#include <interrupt/desc.h>

#define hal_cpu_mxNum	128

#define hal_cpu_intr_Schedule   0x80
#define hal_cpu_intr_Pause      0x81
#define hal_cpu_intr_Execute    0x82

#define hal_cpu_flags_sse42     (1ul << 0)
#define hal_cpu_flags_xsave     (1ul << 1)
#define hal_cpu_flags_osxsave   (1ul << 2)
#define hal_cpu_flags_avx       (1ul << 3)
#define hal_cpu_flags_avx512    (1ul << 4)

typedef struct hal_cpu_Desc {
    u32 x2apic;
    u32 apic;
    u8 *initStk;
    hal_intr_TSS *tss;
    // index of tss64 in gdt table    
    u16 trIdx;
    u8 reserved[4];
    u16 idtTblSz;
    hal_intr_IdtItem *idtTbl;
    intr_Desc *intrDesc[0x40];
    
    u64 curKrlStk;

    u64 flags;
} __attribute__ ((packed)) hal_cpu_Desc;

extern u32 hal_cpu_bspApicId;

extern u8 hal_cpu_apBootEntry[];
extern u8 hal_cpu_apBootEnd[];

#endif