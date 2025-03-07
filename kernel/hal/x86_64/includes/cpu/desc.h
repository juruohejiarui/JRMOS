#ifndef __HAL_CPU_DESC_H__
#define __HAL_CPU_DESC_H__

#include <lib/dtypes.h>
#include <hal/interrupt/desc.h>
#include <interrupt/desc.h>

#define hal_cpu_mxNum	128

typedef struct hal_cpu_Desc {
    u32 x2apic;
    u32 apic;
    // index of tss64 in gdt table    
    u8 *initStk;
    hal_intr_TSS *tss;
    u16 trIdx;
    u8 reserved[4];
    u16 idtTblSz;
    hal_intr_IdtItem *idtTbl;
    intr_Desc *intrInfo[0x40];
} __attribute__ ((packed)) hal_cpu_Desc;

extern u32 hal_cpu_bspApicId;

#endif