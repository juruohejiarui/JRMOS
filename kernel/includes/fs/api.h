#ifndef __FS_API_H__
#define __FS_API_H__

#include <fs/desc.h>

void fs_init();

int fs_installDisk(fs_Disk *disk);

int fs_uninstallDisk(fs_Disk *disk);

#endif