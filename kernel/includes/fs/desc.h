#ifndef __FS_DESC_H__
#define __FS_DESC_H__

#include <lib/dtypes.h>
#include <lib/atomic.h>
#include <hardware/diskdev.h>

typedef struct fs_Disk {
    u64 sz;
    u64 attr;
    Atomic status;

    SafeList partLst;

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

    ListNode lstNd;

    int (*install)(struct fs_Partition *disk);
    int (*uninstall)(struct fs_Partition *disk);
    
    fs_Disk *disk;
} fs_Partition;

#endif