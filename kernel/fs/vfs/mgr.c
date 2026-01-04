#include <fs/vfs/api.h>
#include <lib/string.h>
#include <screen/screen.h>

SafeList fs_vfs_drvLst;

int fs_vfs_init() {
    printk(screen_log, "fs: VFS init\n");
    SafeList_init(&fs_vfs_drvLst);
}

int fs_vfs_registerDriver(fs_vfs_Driver *drv) {
    SafeList_insTail(&fs_vfs_drvLst, &drv->drvLstNd);
    return drv->install ? drv->install() : res_SUCC;
}

int fs_vfs_unregisterDriver(fs_vfs_Driver *drv) {
    register int res = drv->uninstall();
    if (res != res_SUCC) return res;
    SafeList_del(&fs_vfs_drvLst, &drv->drvLstNd);
    return res_SUCC;
}

void fs_vfs_assignName(fs_Partition *par) {
    fs_Disk *disk = par->disk;
    int diskIdx = 0, parIdx = 0;

    SafeList_enum(&fs_diskLst, diskNd) {
        fs_Disk *_disk = container(diskNd, fs_Disk, diskLstNd);
        if (_disk == disk) SafeList_exitEnum(&fs_diskLst);
        diskIdx++;
    }

    SafeList_enum(&disk->parLst, parNd) {
        fs_Partition *_par = container(parNd, fs_Partition, diskParLstNd);
        if (_par == par) SafeList_exitEnum(&disk->parLst);
        parIdx++;
    }
    
    memset(&par->name, 0, fs_vfs_maxParNameLen);
    
    par->name[0] = 'A' + diskIdx;
    par->name[1] = 'A' + diskIdx;
}