#ifndef __HAL_HARDWARE_H__
#define __HAL_HARDWARE_H__

#include <lib/dtypes.h>
#include <hardware/pci.h>

int hal_hw_pci_enum();

int hal_hw_pci_initIntr();

void hal_hw_pci_intr_dispatcher(u64 rsp, u64 num);

int hal_hw_pci_setMsi(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrNum);

int hal_hw_pci_setMsix(hw_pci_MsixCap *cap, hw_pci_Cfg *cfg, intr_Desc *desc, u64 intrNum);

#endif