#ifndef __FS_VFS_DESC_H__
#define __FS_VFS_DESC_H__

#include <fs/desc.h>

#define fs_VFS_maxPathLength 2048
#define fs_VFS_maxNameLength 256

#define fs_VFS_EntryAttr_flags_Readable     0x1
#define fs_VFS_EntryAttr_flags_Writeable    0x2
#define fs_VFS_EntryAttr_flags_Exectuable   0x4
#define fs_VFS_EntryAttr_flags_System       0x8

typedef struct fs_VFS_Entry {
    char path[fs_VFS_maxPathLength];
    char name[fs_VFS_maxNameLength];
    u64 ModiDate;
    u64 crtDate;
    u64 lstOpenDate;

    u64 flags;
} fs_VFS_Entry;

struct fs_VFS_File;
struct fs_VFS_Dir;
struct fs_VFS_Driver;

typedef struct fs_VFS_FileAPI {
    int (*seek)(struct fs_VFS_File *file, u64 ptr);
    int (*read)(struct fs_VFS_File *file, void *buf, u64 len);
    int (*write)(struct fs_VFS_File *file, void *buf, u64 len);
    int (*close)(struct fs_VFS_File *file);
} fs_VFS_FileAPI;

typedef struct fs_VFS_DirAPI {
    int (*nxt)(struct fs_VFS_Dir *dir, char *name);
    int (*createFile)(struct fs_VFS_Dir *dir, char *name);
    int (*createDir)(struct fs_VFS_Dir *dir, char *name);
    int (*delete)(struct fs_VFS_Dir *dir, char *name);
    int (*close)(struct fs_VFS_Dir *dir);
} fs_VFS_DirAPI;

typedef struct fs_VFS_File {
    fs_VFS_Entry *attr;
    fs_VFS_FileAPI *api;
	struct fs_VFS_Driver *driver;
	fs_Partition *par;
} fs_VFS_File;

typedef struct fs_Dir {
    fs_VFS_Entry *attr;
    fs_VFS_DirAPI *api;
	struct fs_VFS_Driver *driver;
	fs_Partition *par;
} fs_VFS_Dir;

typedef struct fs_VFS_Driver {
	char name[fs_VFS_maxNameLength];
	void (*lookup)(fs_VFS_Driver *drv, fs_VFS_Entry *cur, char *name, fs_VFS_Entry *output);
	void (*open)(fs_VFS_Driver *drv, fs_VFS_Entry *ent, fs_VFS_File *file);
	void (*create)(fs_VFS_Driver *drv, fs_VFS_Entry *dir, char *name);
	void (*delete)(fs_VFS_Driver *drv, char *name);

	void (*openDir)(fs_VFS_Driver *drv, fs_VFS_Entry *ent, fs_VFS_Dir *dir);
	void (*createDir)(fs_VFS_Driver *drv, fs_VFS_Entry *dir, char *name);
	void (*deleteDir)(fs_VFS_Driver *drv, char *name);

	void (*uninstall)(fs_VFS_Driver *drv);
} fs_VFS_Driver;

typedef struct fs_VFS_Root {
	fs_VFS_Entry *rootEntry;
	fs_VFS_Driver *driver;
	fs_Partition *par;
	char name[fs_VFS_maxNameLength];
} fs_VFS_Root;
#endif