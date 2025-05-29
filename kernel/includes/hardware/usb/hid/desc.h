#ifndef __HARDWARD_USB_HID_DESC_H__
#define __HARDWARD_USB_HID_DESC_H__

#include <lib/dtypes.h>
#include <hardware/usb/xhci/desc.h>

typedef struct hw_hid_Driver {
	hw_usb_xhci_Device *dev;
} hw_hid_Driver;

#endif