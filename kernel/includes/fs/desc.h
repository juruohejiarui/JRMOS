#ifndef __FS_DESC_H__
#define __FS_DESC_H__

#include <lib/dtypes.h>
#include <lib/atomic.h>
#include <hardware/diskdev.h>
#include <fs/gpt/desc.h>

typedef struct fs_DirEntry {
    u64 attr;
    u8 name[0];
} fs_DirEntry;

typedef struct fs_File {
    fs_DirEntry entry;
    int (*seek)(u64 ptr);
    u64 (*write)(u8 *dt, u64 size);
    u64 (*read)(u8 *dt, u64 size);

    int (*close)(struct fs_File *file);
    struct fs_Disk *disk;
} fs_File;

typedef struct fs_Dir {
    fs_DirEntry entry;
    // get next entry
    int (*nxtEntry)(struct fs_Dir *dir, fs_DirEntry *dirEntry);
    int (*close)(struct fs_Dir *dir);
} fs_Dir;

typedef struct fs_Disk {
    u64 sz;
    u64 st, ed;
    u64 attr;
    Atomic status;

    u64 openCnt;

    int (*open)(const u8 *path, fs_File *file);
    int (*openDir)(const u8 *path, fs_Dir *dir);

    int (*install)(struct fs_Disk *disk);
    int (*uninstall)(struct fs_Disk *disk);

    hw_DiskDev *diskDev;
    // partition manager
    union {
        fs_gpt_Tbl gptTbl;
    };
} fs_Disk;

#define fs_attr_ReadOnly    0x1
#define fs_attr_MultiUser   0x2
#define fs_attr_Journal     0x4
#define fs_attr_IsFile      0x8

#define fs_status_OnUse     0x1

extern SafeList fs_diskLst;

#endif