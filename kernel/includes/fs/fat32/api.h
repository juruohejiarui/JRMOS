#ifndef __FS_FAT32_API_H__
#define __FS_FAT32_API_H__

#include <fs/fat32/desc.h>
#include <fs/gpt/desc.h>

int fs_fat32_init();

int fs_fat32_chkGpt(fs_Disk *disk, fs_gpt_ParEntry *entry);

fs_Partition *fs_fat32_installGptPar(fs_Disk *disk, fs_gpt_ParEntry *entry);

#endif