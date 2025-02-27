#ifndef __HAL_INTERRUPT_H__
#define __HAL_INTERRUPT_H__

#include <lib/dtypes.h>

#define HAL_INTR_MASK
#define HAL_INTR_UNMASK
#define HAL_INTR_PAUSE
#define HAL_INTR_STATE

#define hal_intr_mask()     do { __asm__ volatile ("cli \n\t" : : : "memory"); } while(0)
#define hal_intr_unmask()   do { __asm__ volatile ("sti \n\t" : : : "memory"); } while(0)
#define hal_intr_pause()    do { __asm__ volatile ("hlt \n\t" : : : "memory"); } while(0)

#define hal_intr_state() ({ \
    u64 rflags; \
    __asm__ volatile (\
        "pushfq     \n\t" \
        "popq %0    \n\t" \
        : "=r"(rflags) \
        : \
        : "memory" \
    ); \
    (rflags >> 9) & 1; \
})


void hal_intr_init();
#endif