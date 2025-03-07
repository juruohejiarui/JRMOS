#ifndef __INTERRUPT_DESC_H__
#define __INTERRUPT_DESC_H__

#include <lib/dtypes.h>
#include <hal/interrupt/desc.h>

struct intr_Desc;

struct intr_Ctrl {
    int (*install)(struct intr_Desc *desc, void *arg);
    int (*uninstall)(struct intr_Desc *desc);

    int (*enable)(struct intr_Desc *desc);
    int (*disable)(struct intr_Desc *desc);

    void (*ack)(struct intr_Desc *desc);
};
typedef struct intr_Ctrl intr_Ctrl;

typedef struct intr_Desc {
    u8 cpuId, vecId;
    u64 param;
    char *name;
    void (*handler)(u64 param);
    intr_Ctrl *ctrl;
} intr_Desc;

#define intr_handlerDeclare(name) void name(u64 param)

#endif