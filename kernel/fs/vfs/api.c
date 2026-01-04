#include <fs/vfs/api.h>
#include <fs/api.h>
#include <lib/string.h>
#include <screen/screen.h>
#include <mm/mm.h>

// find partition and assign TO to first ``fs_vfs_Separator``
fs_Partition *_findPar(u16 *path, i32 *to) {
    *to = 0;
    while (path[++*to] != fs_vfs_Separator && path[*to] != '\0') ;
    fs_Partition *res = NULL;
    SafeList_enum(&fs_diskLst, diskNd) {
        fs_Disk *disk = container(diskNd, fs_Disk, diskLstNd);
        SafeList_enum(&disk->parLst, parNd) {
            fs_Partition *par = container(parNd, fs_Partition, diskParLstNd);
            // printk(screen_log, "this partition: %S\n", par->name);
            int parNameLen = strlen16(par->name);
            // printk(screen_log, "length: %d\n", parNameLen);
            if (parNameLen != *to) continue;
            if (!memcmp(par->name, path, parNameLen * sizeof(u16))) {
                res = par;
                SafeList_exitEnum(&disk->parLst);
            }
        }
        if (res != NULL) SafeList_exitEnum(&fs_diskLst);
    }
    return res;
}

fs_vfs_Entry *fs_vfs_lookup(u16 *path) {
    // printk(screen_log, "fs: vfs: lookup: %S\n", path);
    i32 ptr, pathLen = strlen16(path);
    fs_Partition *par = _findPar(path, &ptr);
    if (par == NULL) {
        printk(screen_err, "fs: vfs: partition of %S not found.\n", path);
        return NULL;
    }

    // printk(screen_log, "fs: vfs: found partition %S: %p\n", path, par);

    fs_vfs_Driver *drv = par->drv;
    fs_vfs_Entry *curEnt = drv->getRootEntry(par), *nxtEnt;

    u16 buf[fs_vfs_maxNameLen];
    i32 bufLen;

    while (ptr < pathLen) {
        i32 lstPtr = ++ptr;
        // printk(screen_log, "%d %d\n", ptr, lstPtr);
        for (; ptr < pathLen && path[ptr] != fs_vfs_Separator; ptr++);
        memcpy(path + lstPtr, buf, (ptr - lstPtr) * sizeof(u16));
        buf[ptr - lstPtr] = '\0';
        // printk(screen_log, "fs: vfs: current buf: %S\n", buf);
        
        nxtEnt = drv->lookup(curEnt, buf);

        if (nxtEnt == NULL) goto lookup_fail;
        drv->releaseEntry(curEnt);
        curEnt = nxtEnt;
    }
    return curEnt;
    lookup_fail:
    printk(screen_err, "fs: vfs: failed to find entry %S in directory /%S in Partition %S\n", 
                buf, curEnt->path, par->name);
    drv->releaseEntry(curEnt);
    return NULL;
}

fs_vfs_Dir *fs_vfs_openDir(fs_vfs_Entry *ent) {
    // printk(screen_log, "fs: vfs: open dir %S\n", ent->path);
    fs_Partition *par = (void *)ent->par;
    
    fs_vfs_Dir *dir = par->drv->openDir(ent);

    if (dir == NULL) {
        printk(screen_err, "fs: vfs: failed to open directory %S in partition %S\n", ent->path, par->name);
        return NULL;
    }
    // printk(screen_succ, "fs: vfs: succ to open dir %S\n", ent->path);

    fs_vfs_releaseEnt(ent);

    return dir;
}

int fs_vfs_closeDir(fs_vfs_Dir *dir) {
    return mm_kfree(dir, 0);
}

int fs_vfs_releaseEnt(fs_vfs_Entry *ent) {
    fs_vfs_Driver *drv = ent->par->drv;
    return drv->releaseEntry(ent);
}