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

typedef struct fs_vfs_Entry {
    char path[fs_vfs_maxPathLength];
    char name[fs_vfs_maxNameLength];
    u64 ModiDate;
    u64 crtDate;
    u64 lstOpenDate;

    u64 flags;
} fs_vfs_Entry;

struct fs_vfs_File;
struct fs_vfs_Dir;
struct fs_vfs_Driver;

typedef struct fs_vfs_FileAPI {
    int (*seek)(struct fs_vfs_File *file, u64 ptr);
    int (*read)(struct fs_vfs_File *file, void *buf, u64 len);
    int (*write)(struct fs_vfs_File *file, void *buf, u64 len);
    int (*close)(struct fs_vfs_File *file);
} fs_vfs_FileAPI;

typedef struct fs_vfs_DirAPI {
    int (*nxt)(struct fs_vfs_Dir *dir, char *name);
    int (*createFile)(struct fs_vfs_Dir *dir, char *name);
    int (*createDir)(struct fs_vfs_Dir *dir, char *name);
    int (*delete)(struct fs_vfs_Dir *dir, char *name);
    int (*close)(struct fs_vfs_Dir *dir);
} fs_vfs_DirAPI;

typedef struct fs_vfs_File {
    fs_vfs_Entry *attr;
    fs_vfs_FileAPI *api;
	struct fs_vfs_Driver *driver;
	struct fs_Partition *par;
} fs_vfs_File;

typedef struct fs_Dir {
    fs_vfs_Entry *attr;
    fs_vfs_DirAPI *api;
	struct fs_vfs_Driver *driver;
	struct fs_Partition *par;
} fs_vfs_Dir;

typedef struct fs_vfs_Driver fs_vfs_Driver;

typedef struct fs_vfs_Driver {
	char name[fs_vfs_maxNameLength];
	int (*lookup)(fs_vfs_Entry *cur, char *name, fs_vfs_Entry *output);
	int (*open)(fs_vfs_Entry *ent, fs_vfs_File *file);
	int (*create)(fs_vfs_Entry *dir, char *name);
	int (*delete)(char *name);

    int (*openDir)(fs_vfs_Entry *ent, fs_vfs_Dir *dir);
	int (*createDir)(fs_vfs_Entry *dir, char *name);
	int (*deleteDir)(char *name);

    struct fs_Partition *(*installGptPar)(struct fs_Disk *disk, struct fs_gpt_ParEntry *ent);
    int (*uninstallGptPar)(struct fs_Partition *par);

    int (*install)();
	int (*uninstall)();

    int (*chkGpt)(struct fs_Disk *disk, struct fs_gpt_ParEntry *ent);

    ListNode drvLstNd;

    SafeList parLst;
} fs_vfs_Driver;

extern SafeList fs_vfs_drvLst;

#endif