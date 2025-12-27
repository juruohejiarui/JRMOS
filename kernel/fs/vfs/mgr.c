#include <fs/vfs/api.h>
#include <screen/screen.h>

SafeList fs_vfs_drvLst;

int fs_vfs_init() {
    printk(screen_log, "fs: VFS init\n");
    SafeList_init(&fs_vfs_drvLst);
}

int fs_vfs_registerDriver(fs_vfs_Driver *drv) {
    SafeList_insTail(&fs_vfs_drvLst, &drv->drvLstNd);
    return drv->install();
}

int fs_vfs_unregisterDriver(fs_vfs_Driver *drv) {
    register int res = drv->uninstall();
    if (res != res_SUCC) return res;
    SafeList_del(&fs_vfs_drvLst, &drv->drvLstNd);
    return res_SUCC;
}