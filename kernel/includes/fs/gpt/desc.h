#ifndef __FS_GPT_DESC_H__
#define __FS_GPT_DESC_H__

#include <lib/dtypes.h>

typedef struct fs_gpt_Hdr {
	u8 signature[8];
	u32 revision;
	u32 hdrSz;
	u32 hdrCrc32;
	u32 reserved;
	u64 myLba;
	u64 alterLba;
	u64 firUsableLba;
	u64 lstUsableLba;
	u64 diskGUID[2];
	u64 parEntryLba;
	u32 numOfParEntries;
	u32 szOfParEntry;
	u32 parEntryArrCrc32;
} __attribute__ ((packed)) fs_gpt_Hdr;

typedef struct fs_gpt_ParEntry {
	u64 parTypeGuid[2];
	u64 uniParGuid[2];

	u64 stLba;
	u64 edLba;

	u64 attr;
	
	u8 parName[72];
} __attribute__ ((packed)) fs_gpt_ParEntry;

typedef struct fs_gpt_Tbl {
	fs_gpt_Hdr hdr;
	fs_gpt_ParEntry entry[0];
} fs_gpt_Tbl;

#define fs_gpt_parTypeGuid_None			0x0

#define fs_gpt_parTypeGuid_EfiSysPar0	0x11d2f81fc12a7328
#define fs_gpt_parTypeGuid_EfiSysPar1	0x3bc93ec9a0004bba

#define fs_gpt_parTypeGuid_LinuxFSDt0	0x477284830fc63daf
#define fs_gpt_parTypeGuid_LinuxFSDt1	0xe47d47d8693d798e

#define fs_gpt_parTypeGuid_MsBasicDt0	0x4433b9e5ebd0a0a2
#define fs_gpt_parTypeGuid_MsBasicDt1	0x68b6b72699c787c0

#endif