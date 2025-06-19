#ifndef __INTERRUPT_INTERRUPT_H__
#define __INTERRUPT_INTERRUPT_H__

#include <hal/interrupt/api.h>
#include <interrupt/desc.h>

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

void intr_initDesc(intr_Desc *desc, void (*handler)(u64), u64 param, char *name, intr_Ctrl *ctrl);

int intr_alloc(intr_Desc *desc, int intrNum);

int intr_free(intr_Desc *desc, int intrNum);

int intr_init();

__always_inline__ int intr_enable(intr_Desc *desc) {
	return desc->ctrl->enable(desc);
}
__always_inline__ int intr_disable(intr_Desc *desc) {
	return desc->ctrl->disable(desc);
}

__always_inline__ int intr_install(intr_Desc *desc, void *installArg) { 
	return desc->ctrl->install(desc, installArg);
}
__always_inline__ int intr_uninstall(intr_Desc *desc) {
	return desc->ctrl->uninstall(desc);
}

#endif