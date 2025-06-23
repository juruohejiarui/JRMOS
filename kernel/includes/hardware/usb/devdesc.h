#ifndef __HARDWARE_USB_DEVDESC_H__
#define __HARDWARE_USB_DEVDESC_H__

#include <lib/dtypes.h>

typedef struct hw_usb_devdesc_Hdr {
	u8 len;
	u8 tp;
} __attribute__ ((packed)) hw_usb_devdesc_Hdr;

#define hw_usb_devdesc_Tp_Dev		0x01
#define hw_usb_devdesc_Tp_Cfg 		0x02
#define hw_usb_devdesc_Tp_Str		0x03
#define hw_usb_devdesc_Tp_Inter		0x04
#define hw_usb_devdesc_Tp_Ep		0x05
#define hw_usb_devdesc_Tp_HID		0x21
#define hw_usb_devdesc_Tp_Report	0x22

typedef struct hw_usb_devdesc_Device {
	hw_usb_devdesc_Hdr hdr;
	u16 bcdUsb;
	u8 bDevCls;
	u8 bDevSubCls;
	u8 bDevProtocol;
	u8 bMxPackSz0;
	u16 idVVendor;
	u16 idProduct;
	u16 bcdDev;
	u8 iManufacturer;
	u8 iProduct;
	u8 iSerialNum;
	u8 bNumCfg;
} __attribute__ ((packed)) hw_usb_devdesc_Device;

typedef struct hw_usb_devdesc_Cfg {
	hw_usb_devdesc_Hdr hdr;
	u16 wTotLen;
	u8 bNumInter;
	u8 bCfgVal;
	u8 iCfg;
	u8 bmAttr;
	u8 bMxPw;
} __attribute__ ((packed)) hw_usb_devdesc_Cfg;

typedef struct hw_usb_devdesc_Inter {
	hw_usb_devdesc_Hdr hdr;
	u8 bIntrNum;
	u8 bAlterSet;
	u8 bNumEp;
	u8 bInterCls;
	u8 bInterSubCls;
	u8 bInterProtocol;
	u8 iIntr;
} __attribute__ ((packed)) hw_usb_devdesc_Inter;

typedef struct hw_usb_devdesc_Ep {
	hw_usb_devdesc_Hdr hdr;
	u8 bEpAddr;
	u8 bmAttr;
	u16 wMxPktSz;
	u8 bInterval;
} __attribute__ ((packed)) hw_usb_devdesc_Ep;

typedef struct hw_usb_devdesc_Hid {
	hw_usb_devdesc_Hdr hdr;
	u16 bcdHid;
	u8 bCountryCode;
	u8 bNumDesc;
	struct {
		u8 bDescTp;
		u16 wDescLen;
	} __attribute__ ((packed)) desc[1];
} __attribute__ ((packed)) hw_usb_devdesc_Hid;

__always_inline__ int hw_usb_devdesc_Ep_epId(hw_usb_devdesc_Ep *ep) {
	return ((ep->bEpAddr & 0xf) << 1) + (ep->bEpAddr >> 7);
}

__always_inline__ int hw_usb_devdesc_Ep_epTp(hw_usb_devdesc_Ep *ep) { return (ep->bmAttr & 0x3) | ((ep->bEpAddr >> 5) & 0x4); }

__always_inline__ hw_usb_devdesc_Hdr *hw_usb_devdesc_getNxt(hw_usb_devdesc_Cfg *cfg, hw_usb_devdesc_Hdr *cur) {
	if (!cur) return (hw_usb_devdesc_Hdr *)((u64)cfg + cfg->hdr.len);
	if ((u64)cur + cur->len >= (u64)cfg + cfg->wTotLen) return NULL;
	return (hw_usb_devdesc_Hdr *)((u64)cur + cur->len);
}

#endif