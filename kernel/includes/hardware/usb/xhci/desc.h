#ifndef __HARDWARD_USB_XHCI_DESC_H__
#define __HARDWARE_USB_XHCI_DESC_H__

#include <hal/hardware/reg.h>
#include <hardware/pci.h>
#include <interrupt/desc.h>

typedef struct hw_xhci_Mgr {
    hw_pci_Dev *pci;

    union {
        hw_pci_MsiCap *msiCap;
        hw_pci_MsixCap *msixCap;
    };

    intr_Desc *intr;

    u64 intrNum;
} hw_xhci_Mgr;

#endif