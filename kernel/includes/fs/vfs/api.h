#ifndef __FS_VFS_API_H__
#define __FS_VFS_API_H__

#include <fs/vfs/desc.h>

int fs_VFS_init();

int fs_VFS_registerDriver(fs_VFS_Driver *drv);

int fs_VFS_unregisterDriver(fs_VFS_Driver *drv);

fs_VFS_Root *fs_VFS_createRoot(fs_Partition *par);

#endif