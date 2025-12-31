#ifndef __FS_vfs_DESC_H__
#define __FS_vfs_DESC_H__

#include <lib/dtypes.h>
#include <lib/list.h>

#define fs_vfs_Separator    '/'

#define fs_vfs_maxPathLen       2048
#define fs_vfs_maxNameLen       256
#define fs_vfs_maxPartitionName 32

#define fs_vfs_EntryAttr_flags_Readable     0x1
#define fs_vfs_EntryAttr_flags_Writeable    0x2
#define fs_vfs_EntryAttr_flags_Exectuable   0x4
#define fs_vfs_EntryAttr_flags_System       0x8
#define fs_vfs_EntryAttr_flags_IsDir        0x10

struct fs_vfs_File;
struct fs_vfs_Dir;
struct fs_vfs_Driver;
struct fs_gpt_ParEntry;
struct fs_Disk;
struct fs_Partition;

// # fs_vfs_Entry
// ## Inherent
// Any FS should put this struct at the beginning of definition of entry
// ## Allocation
// - for non-root entry, should be allocate with ``kmalloc(..., 0, drv->releaseEntry)``
// - otherwise, shoule be allocate with`` kmalloc(..., mm_Attr_Shared, NULL)`` and 
// released manually with ``kfree(..., mm_Attr_Shared)`` on ``drv->uninstallXXXPar``
typedef struct fs_vfs_Entry {
    u16 path[fs_vfs_maxPathLen];
    
    u64 ModiTime;
    u64 crtTime;
    u64 lstOpenTime;

    u64 size;

    u64 flags;

    struct fs_partition *par;
} fs_vfs_Entry;

typedef struct fs_vfs_FileAPI {
    int (*seek)(struct fs_vfs_File *file, u64 ptr);
    int (*read)(struct fs_vfs_File *file, void *buf, u64 len);
    int (*write)(struct fs_vfs_File *file, void *buf, u64 len);
} fs_vfs_FileAPI;

typedef struct fs_vfs_DirAPI {
    fs_vfs_Entry (*nxt)(struct fs_vfs_Dir *dir);
} fs_vfs_DirAPI;

// should be created with kmalloc(..., 0, drv->closeFile)
typedef struct fs_vfs_File {
    fs_vfs_Entry *ent;
    fs_vfs_FileAPI *api;

	struct fs_Partition *par;

    ListNode tskLstNd, parLstNd;
    u64 ptr;
} fs_vfs_File;

// should be created with kmalloc(..., 0, drv->closeDir)
typedef struct fs_Dir {
    fs_vfs_Entry *ent;
    fs_vfs_DirAPI *api;
    
	struct fs_Partition *par;

    ListNode tskLstNd, parLstNd;
} fs_vfs_Dir;

typedef struct fs_vfs_Driver fs_vfs_Driver;

typedef struct fs_vfs_Driver {
	char name[fs_vfs_maxNameLen];
    // create a new entry (sub entry) unber CUR
	fs_vfs_Entry *(*lookup)(fs_vfs_Entry *cur, u16 *name);

    // open file with an entry
    // will release the entry after calling this function
	fs_vfs_File *(*openFile)(fs_vfs_Entry *ent);
    // open a directory with an entry
    // will relase the entry after calling this function by drv->releaseEntry
    fs_vfs_Dir *(*openDir)(fs_vfs_Entry *ent);

    int (*closeFile)(fs_vfs_File *file);
    int (*closeDir)(fs_vfs_Dir *dir);

    int (*createFile)(fs_vfs_Entry *dir, u16 *name);
	int (*createDir)(fs_vfs_Entry *dir, u16 *name);
    
    int (*delete)(fs_vfs_Entry *entry);

    struct fs_Partition *(*installGptPar)(struct fs_Disk *disk,  struct fs_gpt_ParEntry *ent);

    int (*uninstallGptPar)(struct fs_Partition *par);

    int (*install)();
	int (*uninstall)();

    int (*chkGpt)(struct fs_Disk *disk, struct fs_gpt_ParEntry *ent);

    fs_vfs_Entry *(*getRootEntry)(struct fs_Partition *par);

    int (*releaseEntry)(fs_vfs_Entry *entry);

    ListNode drvLstNd;

    SafeList parLst;
} fs_vfs_Driver;

extern SafeList fs_vfs_drvLst;

#endif