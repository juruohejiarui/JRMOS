#ifndef __HAL_HARDWARE_uefi_H__
#define __HAL_HARDWARE_uefi_H__

#include <lib/dtypes.h>
#include <screen/screen.h>

// ACPI2.0 GUID
#define hal_hw_uefi_GUID_ACPI2_data1 0x8868e871
#define hal_hw_uefi_GUID_ACPI2_data2 0xe4f1
#define hal_hw_uefi_GUID_ACPI2_data3 0x11d3
#define hal_hw_uefi_GUID_ACPI2_data4 {0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81}

typedef struct hal_hw_uefi_GUID {
	u32 data1;
	u16 data2;
	u16 data3;
	u8 data4[8];
} hal_hw_uefi_GUID;

typedef struct hal_hw_uefi_AcpiHeader {
	u8 signature[4];
	u32 length;
	u8 revision;
	u8 chkSum;
	u8 OEMID[6];
	u8 OEMTableID[8];
	u32 OEMRevision;
	u32 creatorID;
	u32 creatorRevision;
} __attribute__ ((packed)) hal_hw_uefi_AcpiHeader;

typedef struct hal_hw_uefi_MadtEntry {
	u8 type;
	u8 len;
	union {
		struct hal_hw_uefi_MadtEntry_Type0 {
			u8 procId;
			u8 apicId;
			u32 flags;
		} __attribute__ ((packed)) type0;
		struct hal_hw_uefi_MadtEntry_Type1 {
			u8 ioApicId;
			u8 reserved;
			u32 ioApicAddr;
			u32 gloSysIntrBase;
		} __attribute__ ((packed)) type1;
		struct hal_hw_uefi_MadtEntry_Type2 {
			u8 busSrc;
			u8 irqSrc;
			u32 gloSysIntr;
			u16 flags;
		} __attribute__ ((packed)) type2;
		struct hal_hw_uefi_MadtEntry_Type3 {
			u8 nmiSrc;
			u8 reserved;
			u16 flags;
			u32 gloSysIntrBase;
		} __attribute__ ((packed)) type3;
		struct hal_hw_uefi_MadtEntry_Type4 {
			u8 procId;
			u16 flags;
			u8 lint;
		} __attribute__ ((packed)) type4;
		struct hal_hw_uefi_MadtEntry_Type5 {
			u16 reserved;
			u64 addr;
		} __attribute__ ((packed)) type5;
		struct hal_hw_uefi_MadtEntry_Type9 {
			u16 reserved;
			u32 x2apicId;
			u32 flags;
			u32 apicId;
		} __attribute__ ((packed)) type9;
	} __attribute__ ((packed));
} __attribute__ ((packed)) hal_hw_uefi_MadtEntry;

typedef struct hal_hw_uefi_MadtDesc {
	hal_hw_uefi_AcpiHeader hdr;
	u32 locApicAddr;
	u32 flags;
	hal_hw_uefi_MadtEntry entry[0];
} __attribute__ ((packed)) hal_hw_uefi_MadtDesc;

typedef struct hal_hw_uefi_RsdpDesc {
	u8 signature[8];
	u8 chkSum;
	u8 OEMID[6];
	u8 revision;
	u32 RsdtAddress;
	u32 length;
	u64 xsdtAddr;
	u8 extendedChkSum;
	u8 reserved[3];
} __attribute__ ((packed)) hal_hw_uefi_RsdpDesc;

// ACPI2.0 XSDT
typedef struct hal_hw_uefi_XsdtDesc{
	hal_hw_uefi_AcpiHeader hdr;
	u64 entry[0];
} __attribute__ ((packed)) hal_hw_uefi_XsdtDesc;

typedef struct hal_hw_uefi_E820Desc {
	u64 addr;
	u64 len;
	u32 type;
} __attribute__ ((packed)) hal_hw_uefi_E820Decs;

typedef struct hal_hw_uefi_E820DescArray {
	u32 entryCnt;
	hal_hw_uefi_E820Decs entry[0];
} hal_hw_uefi_E820DescArray;

typedef struct hal_hw_uefi_CfgTbl {
	hal_hw_uefi_GUID venderGUID;
	void *vendorTbl;
} __attribute__ ((packed)) hal_hw_uefi_CfgTbl;

// all the information from uefi can be found in this structure
typedef struct hal_hw_uefi_InfoStruct {
	u64 cfgTblCnt;
	hal_hw_uefi_CfgTbl *cfgTbls;
	screen_Info screenInfo;
	hal_hw_uefi_E820DescArray e820Array;
	u64 memDesc;
	u64 memDescSize;
} hal_hw_uefi_InfoStruct;

extern hal_hw_uefi_RsdpDesc *hal_hw_uefi_rsdpTbl;
extern hal_hw_uefi_XsdtDesc *hal_hw_uefi_xsdtTbl;

extern hal_hw_uefi_InfoStruct *hal_hw_uefi_info;

void hal_hw_uefi_init();

int hal_hw_uefi_loadTbl();

#endif