#ifndef __INTERRUPT_INTERRUPT_H__
#define __INTERRUPT_INTERRUPT_H__

#include <hal/interrupt/interrupt.h>

#ifdef HAL_INTR_MASK
#define intr_mask hal_intr_mask
#else
#error no definition of intr_mask() for this arch
#endif

#ifdef HAL_INTR_UNMASK
#define intr_unmask hal_intr_unmask
#else
#error no definition of intr_unmask() for this arch
#endif

#ifdef HAL_INTR_STATE
#define intr_state hal_intr_state
#else
#error no definition of intr_mask() for this arch
#endif

#ifdef HAL_INTR_PAUSE
#define intr_pause hal_intr_pause
#else
#error no definition of intr_pause() for this arch
#endif

void intr_init();

#endif