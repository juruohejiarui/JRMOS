#ifndef __HAL_INTR_TRAP_H__
#define __HAL_INTR_TRAP_H__

extern void hal_intr_divideError();
extern void hal_intr_debug();
extern void hal_intr_nmi();
extern void hal_intr_int3();
extern void hal_intr_overflow();
extern void hal_intr_bounds();
extern void hal_intr_undefinedOpcode();
extern void hal_intr_devNotAvailable();
extern void hal_intr_doubleFault();
extern void hal_intr_coprocessorSegmentOverrun();
extern void hal_intr_invalidTSS();
extern void hal_intr_segmentNotPresent();
extern void hal_intr_stackSegmentFault();
extern void hal_intr_generalProtection();
extern void hal_intr_pageFault();
extern void hal_intr_x87FPUError();
extern void hal_intr_alignmentCheck();
extern void hal_intr_machineCheck();
extern void hal_intr_simdError();
extern void hal_intr_virtualizationError();

void hal_intr_initTrapGates();

#endif