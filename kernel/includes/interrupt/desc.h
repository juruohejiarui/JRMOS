#ifndef __INTERRUPT_DESC_H__
#define __INTERRUPT_DESC_H__

#include <lib/dtypes.h>
#include <hal/interrupt/desc.h>

struct intr_Info {
    u32 cpuId;
    u32 vecId;
    u64 param;
    void (*handler)(u32 vec, u64 param);
    void (*enable)(u32 vec);
    void (*disable)(u32 vec);
    hal_intr_Info halInfo;
};

typedef struct intr_Info;


#endif