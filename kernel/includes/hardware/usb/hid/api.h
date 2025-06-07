#ifndef __HARDWARE_USB_HID_API_H__
#define __HARDWARE_USB_HID_API_H__

#include <hardware/usb/hid/desc.h>
#include <hardware/hid/parse.h>

void hw_usb_hid_init();

int hw_usb_hid_check(hw_Device *dev);

int hw_usb_hid_keyboard(hw_usb_xhci_Device *dev, hw_hid_Parser *parser, int inEp, int outEp);

int hw_usb_hid_mouse(hw_usb_xhci_Device *dev, hw_hid_Parser *parser, int inEp);

int hw_usb_hid_setReport(hw_usb_xhci_Device *dev, u8 *report, u32 reportSz, u8 reportId, u8 interId);

#endif