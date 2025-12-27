#ifndef __FS_DESC_H__
#define __FS_DESC_H__

#include <lib/dtypes.h>
#include <lib/atomic.h>
#include <hardware/diskdev.h>
#include <fs/vfs/desc.h>

typedef struct fs_Disk {
    u64 sz;
    u64 attr;
    Atomic status;

    SafeList parLst;

    hw_DiskDev *device;
} fs_Disk;

struct fs_File;

typedef struct fs_Partition {
    u64 sz;
    u64 st, ed;
    u64 attr;
    Atomic status;
    Atomic openCnt;

    SafeList dirLst, fileLst;

    // list node under safe list of fs_Disk
    ListNode diskParLstNd;
    // list node under safe list of fs_vfs_Driver
    ListNode vfsParLstNd;

    // the disk that this partition belongs to
    fs_Disk *disk;

    // the vfs driver of this partition
    fs_vfs_Driver *drv;
    // the vfs root of this partition
    fs_vfs_Entry *root;
    char name[fs_vfs_maxNameLength];
} fs_Partition;

#endif