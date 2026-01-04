#ifndef __FS_FAT32_DESC_H__
#define __FS_FAT32_DESC_H__

#include <fs/desc.h>
#include <lib/rbtree.h>

#define fs_fat32_ClusEnd (~0x0u)
#define fs_fat32_DeleteFlag	0xe5

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
	u16 extFlags;
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

typedef struct fs_fat32_FSInfoSector {
	u32 leadSig;
	u8 reserved1[480];
	u32 structSig;
	u32 freeCnt;
	u32 nxtFree;
	u8 reserved2[12];
	u32 trailSig;
} __attribute__ ((packed)) fs_fat32_FSInfoSector;

typedef struct fs_fat32_DirEntry {
	u8 name[11];
	u8 attr;
	#define fs_fat32_DirEntry_attr_ReadOnly	0x01
	#define fs_fat32_DirEntry_attr_Hidden	0x02
	#define fs_fat32_DirEntry_attr_System	0x04
	#define fs_fat32_DirEntry_attr_VolumeId	0x08
	#define fs_fat32_DirEntry_attr_Dir		0x10
	#define fs_fat32_DirEntry_attr_Archive	0x20
	#define fs_fat32_DirEntry_attr_LongName	0x0f
	u8 nameCase;
	#define fs_fat32_DirEntry_nameCase_LowName		0x8
	#define fs_fat32_DirEntry_nameCase_LowExtName	0x10
	u8 crtTimeTenth;
	u16 crtTime;
	u16 crtDate;
	u16 lstAccData;
	u16 fstClusHi;
	u16 wrtTime;
	u16 wrtData;
	u16 fstClusLo;
	u32 fileSz;
} __attribute__((packed)) fs_fat32_DirEntry;

typedef struct fs_fat32_LDirEntry {
	u8 ord;
	u16 name1[5];
	u8 attr;
	u8 type;
	u8 chksum;
	u16 name2[6];
	u16 fstClusLo;
	u16 name3[2];
} __attribute__ ((packed)) fs_fat32_LDirEntry;

typedef struct fs_fat32_ClusCacheNd {
	void *clus;
	RBNode clusCacheNd;
	ListNode freeClusCacheNd;
	// offset (Cluster)
	u64 off;

	#define fs_fat32_ClusCacheNd_MaxModiCnt 128
	/// @brief when count of modify reach fs_fs_fat32_ClusCacheNd_MaxModiCnt, this contents of this cache will be
	/// written to disk.
	u64 modiCnt;

	/// @brief only one program or service can modify this page cache.
	SpinLock useLck;
	Atomic waitCnt;
} fs_fat32_ClusCacheNd;

typedef struct fs_fat32_FatCacheNd {
	u32 *sec;
	RBNode rbNd;
	// offset (sector)
	u32 off;	
} fs_fat32_FatCacheNd;

typedef struct fs_fat32_Cache {
	RBTree entryTr;
	#define fs_fat32_FreeEntryCache_MaxNum 64
	ListNode freeEntryLst;
	u64 freeEntryNum;


	RBTree fatTr;
	#define fs_fat32_FatCache_MaxNum 64
	u64 fatNum;

	RBTree clusCacheTr;
	#define fs_fat32_FreeClusCache_MaxNum	64
	ListNode freeClusCacheLst;
	u64 freeClusCacheNum;
} fs_fat32_Cache;

typedef struct fs_fat32_Entry {
	fs_vfs_Entry vfsEntry;
	fs_fat32_DirEntry dirEntry;

	// node on fs_fat32_Cache->entryTr
    RBNode cacheNd;
	
	// node on fs_fat32_Cache->freeEntryLst
	ListNode freeCacheNd;

	u64 ref;
} fs_fat32_Entry;

typedef struct fs_fat32_File {
	fs_vfs_File file;
	
	SpinLock lck;

	u64 clusId, clusOff;
} fs_fat32_File;

typedef struct fs_fat32_Dir {
	fs_vfs_Dir dir;

	SpinLock lck;

    u64 clusId, clusOff;
	
	fs_fat32_Entry *curEnt;
} fs_fat32_Dir;

typedef struct fs_fat32_Partition { 
	fs_fat32_BootSector bootSec;
	fs_fat32_FSInfoSector fsSec;

	u64 fstDtSec;
	u64 bytesPerClus;
	u64 lbaPerClus;
	u64 fstFat1Sec;
	u64 fstFat2Sec;

	fs_fat32_Entry *root;

	fs_fat32_Cache cache;
	
	fs_Partition par;
} fs_fat32_Partition;

extern fs_vfs_Driver fs_fat32_drv;

#endif