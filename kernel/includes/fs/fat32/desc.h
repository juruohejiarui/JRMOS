#ifndef __FS_FAT32_DESC_H__
#define __FS_FAT32_DESC_H__

#include <fs/desc.h>

typedef struct fs_fat32_BootSector {
	u8 jmpBoot[3];
	u8 oemName[8];
	u16 bytesPerSec;
	u8 secPerClus;
	u16 rsvdSecCnt;
	u8 numFats;
	u16 rootEntCnt;
	u16 totSec16;
	u8 media;
	u16 fatSz16;
	u16 secPerTrk;
	u16 numHeads;
	u32 hiddSec;
	u32 totSec32;
	u32 fatSz32;
	u16 extFalgs;
	u16 fsVer;
	// root directory start cluster
	u32 rootClus;
	// fsInfo sector id
	u16 fsInfo;
	// backup boot sector id
	u16 bkBootSec;
	u8 reserved[12];
	u8 drvNum;
	u8 reserved1;
	u8 bootSig;
	u32 volId;
	u8 volLab[11];
	u64 fileSysType;
	u8 bootCode[420];
	u16 bootSigEnd;
} __attribute__ ((packed)) fs_fat32_BootSector;

typedef struct fs_fat32_Partition {
	fs_fat32_BootSector bootSec;
	fs_Partition par;
	
} fs_fat32_Partition;
#endif