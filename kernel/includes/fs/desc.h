#ifndef __FS_DESC_H__
#define __FS_DESC_H__

#include <lib/dtypes.h>
#include <lib/atomic.h>
#include <hardware/diskdev.h>

typedef struct fs_Entry {
    u64 attr;
    u8 name[0];
} fs_Entry;

typedef struct fs_File {
    fs_Entry entry;
    ListNode lstNd;
    int (*seek)(u64 ptr);
    u64 (*write)(u8 *dt, u64 size);
    u64 (*read)(u8 *dt, u64 size);

    int (*close)(struct fs_File *file);
    struct fs_Disk *disk;
} fs_File;

typedef struct fs_Dir {
    fs_Entry entry;
    ListNode lstNd;
    // get next entry
    int (*nxtEntry)(struct fs_Dir *dir, fs_Entry *dirEntry);
    int (*close)(struct fs_Dir *dir);
} fs_Dir;

typedef struct fs_Partition {
    u64 sz;
    u64 st, ed;
    u64 attr;
    Atomic status;
    Atomic openCnt;

    SafeList dirLst, fileLst;

    int (*open)(const u8 *path, fs_File *file);
    int (*openDir)(const u8 *path, fs_Dir *dir);

    int (*install)(struct fs_Partition *disk);
    int (*uninstall)(struct fs_Partition *disk);
    
    struct fs_Disk *disk;
    ListNode lst;

} fs_Partition;

typedef struct fs_Disk {
    u64 sz;
    u64 attr;
    Atomic status;

    SafeList partLst;

    hw_DiskDev *device;
} fs_Disk;

#define fs_attr_ReadOnly    0x1
#define fs_attr_MultiUser   0x2
#define fs_attr_Journal     0x4
#define fs_attr_IsFile      0x8

#define fs_status_OnUse     0x1

extern SafeList fs_diskLst;

#endif