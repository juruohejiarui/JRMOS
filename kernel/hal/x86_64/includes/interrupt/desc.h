#ifndef __HAL_INTERRUPT_DESC_H__
#define __HAL_INTERRUPT_DESC_H__

#include <lib/dtypes.h>

typedef struct hal_intr_GdtItem { u8 dt[8]; } hal_intr_GdtItem;
typedef struct hal_intr_IdtItem { u8 dt[16]; } hal_intr_IdtItem;

typedef struct hal_intr_TSS {
    u32 reserved0;
    u64 rsp0, rsp1, rsp2;
    u64 reserved1;
    u64 ist1, ist2, ist3, ist4, ist5, ist6, ist7;
    u64 reserved2;
    u16 reserved3;
    u16 iomapBaseAddr;
} __attribute__((packed)) hal_intr_TSS;

#endif