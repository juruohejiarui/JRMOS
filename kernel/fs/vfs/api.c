#include <fs/vfs/api.h>
#include <fs/api.h>
#include <lib/string.h>
#include <screen/screen.h>
#include <mm/mm.h>

// find partition and assign TO to first ``fs_vfs_Separator``
fs_Partition *_findPar(u16 *path, i32 *to) {
    *to = 0;
    while (path[*to] != fs_vfs_Separator) *to++;
    fs_Partition *res = NULL;
    SafeList_enum(&fs_diskLst, diskNd) {
        fs_Disk *disk = container(diskNd, fs_Disk, diskLstNd);
        SafeList_enum(&disk->parLst, parNd) {
            fs_Partition *par = container(parNd, fs_Partition, diskParLstNd);
            int parNameLen = strlen16(par->name);
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
    i32 ptr, pathLen = strlen16(path);
    fs_Partition *par = _findPar(path, &ptr);
    if (par == NULL) {
        printk(screen_err, "fs: vfs: partition of %S not found.\n", path);
        return NULL;
    }

    fs_vfs_Driver *drv = par->drv;
    fs_vfs_Entry *curEnt = drv->getRootEntry(par), *nxtEnt;

    u16 buf[fs_vfs_maxNameLen];
    i32 bufLen;

    while (ptr < pathLen) {
        i32 lstPtr = ++ptr;
        for (; ptr < pathLen && path[ptr] != fs_vfs_Separator; ptr++);
        memcpy(path, buf, (ptr - lstPtr) * sizeof(u16));
        buf[ptr - lstPtr] = '\0';
        
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
    fs_Partition *par = (void *)ent->par;
    
    fs_vfs_Dir *dir = par->drv->openDir(ent);

    if (dir == NULL) {
        printk(screen_err, "fs: vfs: failed to open directory %S in partition %S\n", ent->path, par->name);
        return NULL;
    }

    SafeList_insTail(&par->dirLst, &dir->parLstNd);
    SafeList_insTail(&task_current->thread->dirLst, &dir->tskLstNd);

    return dir;
}

int fs_vfs_closeDir(fs_vfs_Dir *dir) {
    fs_Partition *par = dir->par;
    SafeList_del(&par->dirLst, &dir->parLstNd);
    SafeList_del(&task_current->thread->dirLst, &dir->tskLstNd);

    return mm_kfree(dir, 0);
}