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

    int (*open)(u8 *path, struct fs_File *file);

    int (*install)(struct fs_Partition *disk);
    int (*uninstall)(struct fs_Partition *disk);
    
    fs_Disk *disk;
} fs_Partition;

#define fs_File_name_MaxLen 256
typedef struct fs_DirEntry {
    u64 attr;
    u64 name[fs_File_name_MaxLen];
} fs_DirEntry;

typedef struct fs_File {
    fs_DirEntry entry;
    ListNode lstNd;
    int (*seek)(struct fs_File *file, u64 ptr);
    u64 (*write)(struct fs_File *file, u8 *dt, u64 size);
    u64 (*read)(struct fs_File *file, u8 *dt, u64 size);

    int (*close)(struct fs_File *file);
    fs_Partition *par;
} fs_File;

typedef struct fs_Dir {
    // only close() can be called
    fs_File file;
    int (*nxtEnt)(fs_DirEntry *outEnt);
    int (*newEnt)(fs_DirEntry *inEnt);
    int (*delEnt)(fs_DirEntry *inEnt);
} fs_Dir;

#define fs_attr_ReadOnly    0x1
#define fs_attr_MultiUser   0x2
#define fs_attr_Journal     0x4
#define fs_attr_IsDir       0x8

#define fs_status_OnUse     0x1

extern SafeList fs_diskLst;

#endif