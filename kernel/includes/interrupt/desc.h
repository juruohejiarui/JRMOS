#ifndef __INTERRUPT_DESC_H__
#define __INTERRUPT_DESC_H__

#include <lib/dtypes.h>
#include <hal/interrupt/desc.h>

struct intr_Ctrl {
    void (*install)(u8 irq, void *arg);
    void (*uninstall)(u8 irq);

    void (*enable)(u8 irq);
    void (*disable)(u8 irq);

    void (*ack)(u8 irq);
};
typedef struct intr_Ctrl intr_Ctrl;

struct intr_Info {
    u8 cpuId, vecId;

    u64 param;

    char *name;
    
    void (*handler)(u64 param);
    intr_Ctrl *ctrl;
};

typedef struct intr_Info intr_Info;

#define intr_handlerDeclare(name) void name(u64 param)

#endif