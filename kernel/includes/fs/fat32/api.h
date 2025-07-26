#ifndef __FS_FAT32_API_H__
#define __FS_FAT32_API_H__

#include <fs/fat32/desc.h>

int fs_fat32_chk(fs_fat32_BootSector *bs);

int fs_fat32_initPar(fs_fat32_Partition *par, fs_fat32_BootSector *bs, fs_Disk *disk, u64 stLba, u64 edLba);
#endif