#ifndef __FS_vfs_DESC_H__
#define __FS_vfs_DESC_H__

#include <lib/dtypes.h>
#include <lib/list.h>

#define fs_vfs_maxPathLength 2048
#define fs_vfs_maxNameLength 256

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
    char path[fs_vfs_maxPathLength];
    char name[fs_vfs_maxNameLength];
    
    u64 ModiTime;
    u64 crtTime;
    u64 lstOpenTime;

    u64 size;

    u64 flags;

    struct fs_partition *par;
} fs_vfs_Entry;

// should be created with kmalloc(..., 0, drv->closeFile)
typedef struct fs_vfs_FileAPI {
    int (*seek)(struct fs_vfs_File *file, u64 ptr);
    int (*read)(struct fs_vfs_File *file, void *buf, u64 len);
    int (*write)(struct fs_vfs_File *file, void *buf, u64 len);
} fs_vfs_FileAPI;

// should be created with kmalloc(..., 0, drv->closeDir)
typedef struct fs_vfs_DirAPI {
    int (*nxt)(struct fs_vfs_Dir *dir, char *name, fs_vfs_Entry *entry);
} fs_vfs_DirAPI;

typedef struct fs_vfs_File {
    fs_vfs_Entry *attr;
    fs_vfs_FileAPI *api;
	struct fs_vfs_Driver *driver;
	struct fs_Partition *par;

    ListNode openFileLstNd;
} fs_vfs_File;

typedef struct fs_Dir {
    fs_vfs_Entry *attr;
    fs_vfs_DirAPI *api;
	struct fs_vfs_Driver *driver;
	struct fs_Partition *par;

    ListNode openDirLstNd;
} fs_vfs_Dir;

typedef struct fs_vfs_Driver fs_vfs_Driver;

typedef struct fs_vfs_Driver {
	char name[fs_vfs_maxNameLength];
    // create a new entry (sub entry) unber CUR 
	fs_vfs_Entry *(*lookup)(fs_vfs_Entry *cur, char *name);

	fs_vfs_File *(*openFile)(fs_vfs_Entry *ent);
    fs_vfs_Dir *(*openDir)(fs_vfs_Entry *ent);

    int (*closeFile)(fs_vfs_File *file);
    int (*closeDir)(fs_vfs_Dir *dir);

    int (*createFile)(fs_vfs_Entry *dir, char *name);
	int (*createDir)(fs_vfs_Entry *dir, char *name);
    
    int (*delete)(fs_vfs_Entry *entry);

    struct fs_Partition *(*installGptPar)(struct fs_Disk *disk,  struct fs_gpt_ParEntry *ent);

    int (*uninstallGptPar)(struct fs_Partition *par);

    int (*install)();
	int (*uninstall)();

    int (*chkGpt)(struct fs_Disk *disk, struct fs_gpt_ParEntry *ent);

    int (*releaseEntry)(fs_vfs_Entry *entry);

    ListNode drvLstNd;

    SafeList parLst;
} fs_vfs_Driver;

extern SafeList fs_vfs_drvLst;

#endif