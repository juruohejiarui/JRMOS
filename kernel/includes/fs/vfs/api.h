#ifndef __FS_vfs_API_H__
#define __FS_vfs_API_H__

#include <fs/vfs/desc.h>
#include <fs/desc.h>
#include <fs/gpt/desc.h>

int fs_vfs_init();

int fs_vfs_registerDriver(fs_vfs_Driver *drv);

int fs_vfs_unregisterDriver(fs_vfs_Driver *drv);
#endif