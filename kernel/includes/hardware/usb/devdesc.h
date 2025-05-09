#ifndef __HARDWARE_USB_DEVDESC_H__
#define __HARDWARE_USB_DEVDESC_H__

#include <lib/dtypes.h>

typedef struct hw_usb_devdesc_Hdr {
	u8 len;
	u8 type;
} __attribute__ ((packed)) hw_usb_devdesc_Hdr;

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

typedef struct hw_usb_devdesc_Intr {
	hw_usb_devdesc_Hdr hdr;
	u8 bIntrNum;
	u8 bAlterSet;
	u8 bNumEp;
	u8 bInterCls;
	u8 bInterSubCls;
	u8 bInterProtocol;
	u8 iIntr;
} __attribute__ ((packed)) hw_usb_devdesc_Intr;

typedef struct hw_usb_devdesc_Ep {
	hw_usb_devdesc_Hdr hdr;
	u8 bEpAddr;
	u8 bmAttr;
	u16 wMxPktSz;
	u8 bInterval;
} __attribute__ ((packed)) hw_usb_devdesc_Ep;

#endif