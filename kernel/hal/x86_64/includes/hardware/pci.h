#ifndef __HAL_HARDWARE_H__
#define __HAL_HARDWARE_H__

#include <lib/dtypes.h>

int hal_hw_pci_enum();

int hal_hw_pci_initIntr();

void hal_hw_pci_intr_dispatcher(u64 rsp, u64 num);

#endif