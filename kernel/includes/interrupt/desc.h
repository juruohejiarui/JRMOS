#ifndef __INTERRUPT_DESC_H__
#define __INTERRUPT_DESC_H__

#include <lib/dtypes.h>
#include <hal/interrupt/desc.h>

struct intr_Ctrl {
    void (*install)(u32 irq, u64 arg);
    void (*uninstall)(u32 irq);

    void (*enable)(u32 irq);
    void (*disable)(u32 irq);

    void (*ack)(u32 irq);
};
typedef struct intr_Ctrl intr_Ctrl;

struct intr_Info {
    u32 cpuId;
    u32 vecId;
    u64 param;
    
    void (*handler)(u32 irq, u64 param);
    intr_Ctrl *ctrl;
};

typedef struct intr_Info intr_Info;

#endif