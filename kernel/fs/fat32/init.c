#include "inner.h"
#include <lib/string.h>
#include <fs/vfs/api.h>
#include <screen/screen.h>

fs_vfs_Driver fs_fat32_drv;

int fs_fat32_initParInfo(fs_fat32_Partition *par) {
	// read FSInfo sector
	u64 res = par->par.disk->device->read(par->par.disk->device, par->par.st + 1, 1, &par->fsSec);
	if (res != 1) return res_FAIL;

	printk(screen_log, 
		"fs: fat32: %p: fs struct:\n"
		"    leadSig: %08x"
		"    struSig: %08x\n"
		"    freeCnt: %08x"
		"    nxtFree: %08x\n"
		"    trailSig:%08x"
		"    rsvdSec: %08x\n"
		"    numFats: %08x\n",
		par, par->fsSec.leadSig, par->fsSec.structSig, par->fsSec.freeCnt, par->fsSec.nxtFree, par->fsSec.trailSig,
		par->bootSec.rsvdSecCnt, par->bootSec.numFats);

	par->fstDtSec = par->bootSec.rsvdSecCnt + par->bootSec.fatSz32 * par->bootSec.numFats;
	par->fstFat1Sec = par->bootSec.rsvdSecCnt;
	par->fstFat2Sec = par->fstFat1Sec + par->bootSec.fatSz32;
	par->bytesPerClus = par->bootSec.secPerClus * par->bootSec.bytesPerSec;
	par->lbaPerClus = par->bytesPerClus / hw_diskdev_lbaSz;

	printk(screen_log, 
		"fs: fat32: %p: info: \n"
		"    fstDtSec:    %016lx fstFat1Sec: %016lx\n"
		"    fstFat2Sec:  %016lx bytesPerSec:%016lx\n"
		"    bytesPerClus:%016lx lbaPerClus: %016lx\n",
		par,
		par->fstDtSec, par->fstFat1Sec, par->fstFat2Sec, par->bootSec.bytesPerSec, par->bytesPerClus, par->lbaPerClus);
	
	return res_SUCC;
}

RBTree_insertDef(fs_fat32_FatCacheNd_insert, fs_fat32_FatCacheNd_cmp);
RBTree_findDef(fs_fat32_FatCacheNd_find, fs_fat32_FatCacheNd_match);

RBTree_insertDef(fs_fat32_ClusCacheNd_insert, fs_fat32_ClusCacheNd_cmp);
RBTree_findDef(fs_fat32_ClusCacheNd_find, fs_fat32_ClusCacheNd_match);

int fs_fat32_initParCache(fs_fat32_Partition *par) {
	RBTree_init(&par->cache.fatTr, fs_fat32_FatCacheNd_insert, fs_fat32_FatCacheNd_find);
	par->cache.fatNum = 0;
	SpinLock_init(&par->cache.fatLck);

    return res_SUCC;
}

int fs_fat32_init() {
	memset(&fs_fat32_drv, NULL, sizeof(fs_vfs_Driver));
	fs_fat32_drv.chkGpt = fs_fat32_chkGpt;
	fs_fat32_drv.installGptPar = fs_fat32_installGptPar;
	fs_vfs_registerDriver(&fs_fat32_drv);
}