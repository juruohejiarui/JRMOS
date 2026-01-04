#ifndef __FS_vfs_API_H__
#define __FS_vfs_API_H__

#include <fs/vfs/desc.h>
#include <fs/desc.h>
#include <fs/gpt/desc.h>

int fs_vfs_init();

int fs_vfs_registerDriver(fs_vfs_Driver *drv);

int fs_vfs_unregisterDriver(fs_vfs_Driver *drv);

// assignName automatically
void fs_vfs_assignName(fs_Partition *par);

fs_vfs_Entry *fs_vfs_lookup(u16 *path);

fs_vfs_Dir *fs_vfs_openDir(fs_vfs_Entry *ent);

int fs_vfs_releaseEnt(fs_vfs_Entry *ent);

int fs_vfs_closeDir(fs_vfs_Dir *dir);



#endif