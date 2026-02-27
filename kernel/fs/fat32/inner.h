#ifndef __FS_FAT32_INNER_H__
#define __FS_FAT32_INNER_H__

#include <fs/fat32/api.h>

extern fs_vfs_Driver fs_fat32_drv;
extern fs_vfs_FileAPI fs_fat32_fileApi;
extern fs_vfs_DirAPI fs_fat32_dirApi;

__always_inline__ u32 fs_fat32_DirEntry_getFstClus(fs_fat32_DirEntry *dir) {
	return ((((u32)dir->fstClusHi) << 16) | dir->fstClusLo);
}

__always_inline__ void fs_fat32_DirEntry_setFstClus(fs_fat32_DirEntry *dir, u32 fstClus) {
	dir->fstClusHi = (fstClus >> 16);
	dir->fstClusLo = (fstClus & 0xffff);
}

fs_fat32_ClusCacheNd *fs_fat32_getClusCacheNd(fs_fat32_Partition *par, u64 idx);

int fs_fat32_releaseClusCacheNd(fs_fat32_Partition *par, fs_fat32_ClusCacheNd *nd, int modi);

u32 fs_fat32_getNxtClus(fs_fat32_Partition *par, u32 clus);

int fs_fat32_setNxtClus(fs_fat32_Partition *par, u32 clus, u32 nxt);

u32 fs_fat32_allocClus(fs_fat32_Partition *par);

int fs_fat32_initParInfo(fs_fat32_Partition *par);

int fs_fat32_initParCache(fs_fat32_Partition *par);

// functions for vfs drivers
fs_vfs_Entry *fs_fat32_lookup(fs_vfs_Entry *cur, u16 *name);

fs_vfs_File *fs_fat32_openFile(fs_vfs_Entry *ent);
fs_vfs_Dir *fs_fat32_openDir(fs_vfs_Entry *ent);

int fs_fat32_closeFile(fs_vfs_File *file);
int fs_fat32_closeDir(fs_vfs_Dir *dir);

int fs_fat32_releaseEntry(fs_vfs_Entry *entry);

int fs_fat32_chkGpt(fs_Disk *disk, fs_gpt_ParEntry *entry);

fs_Partition *fs_fat32_installGptPar(fs_Disk *disk, fs_gpt_ParEntry *entry);

fs_vfs_Entry *fs_fat32_getRootEntry(fs_Partition *par);

// functions for vfs File api
i64 fs_fat32_FileAPI_seek(fs_vfs_File *file, i64 off, int base);
i64 fs_fat32_FileAPI_write(fs_vfs_File *file, void *buf, u64 len);
i64 fs_fat32_FileAPI_read(fs_vfs_File *file, void *buf, u64 len);

// functions for vfs Dir api
fs_vfs_Entry *fs_fat32_DirAPI_nxt(fs_vfs_Dir *dir);

void fs_fat32_dbg(fs_fat32_Partition *par);

#endif