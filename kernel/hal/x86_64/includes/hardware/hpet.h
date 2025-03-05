#ifndef __HAL_HARDWARE_HPET_H__
#define __HAL_HARDWARE_HPET_H__

#include <hal/hardware/uefi.h>

typedef struct hal_hw_hpet_UefiAddr {
	u8 addrSpaceId;
	u8 regBitWidth;
	u8 regBitOffset;
	u8 reserved;
	u64 Address;
} __attribute__ ((packed)) hal_hw_hpet_UefiAddr;

typedef struct hal_hw_hpet_XsdtDesc {
    hal_hw_uefi_AcpiHeader hdr;

    u8 hardwareRevID;			// 36
	u8 comparatorCount : 5;		// 37-41
	u8 counterSize : 1;			// 42
	u8 reserved : 1;			// 43
	u8 legacyReplace : 1; 	    // 44
	u16 pciVendorID;			// 45-46
	hal_hw_hpet_UefiAddr addr; 	// 47-63
	u32 hpetNum;				// 64-67
	u16 minTick;			    // 68-69
	u8 pageProtect;			    // 70
} __attribute__((packed)) hal_hw_hpet_XsdtDesc;

extern hal_hw_hpet_XsdtDesc *hal_hw_hpet_xsdtDesc;

int hal_hw_hpet_init();

#endif