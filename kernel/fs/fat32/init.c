#include "inner.h"
#include <lib/string.h>
#include <fs/vfs/api.h>
#include <screen/screen.h>

fs_vfs_Driver fs_fat32_drv;
fs_vfs_FileAPI fs_fat32_fileApi;
fs_vfs_DirAPI fs_fat32_dirApi;

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
		"    numFats: %08x"
		"    rootCLus:%08x\n",
		par, par->fsSec.leadSig, par->fsSec.structSig, par->fsSec.freeCnt, par->fsSec.nxtFree, par->fsSec.trailSig,
		par->bootSec.rsvdSecCnt, par->bootSec.numFats, par->bootSec.rootClus);

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

__always_inline__ int fs_fat32_ClusCacheNd_cmp(RBNode *a, RBNode *b) {
	register fs_fat32_ClusCacheNd *ta = container(a, fs_fat32_ClusCacheNd, clusCacheNd),
		*tb = container(b, fs_fat32_ClusCacheNd, clusCacheNd);
	return ta->off < tb->off;
}

__always_inline__ int fs_fat32_ClusCacheNd_match(RBNode *a, void *b) {
	register fs_fat32_ClusCacheNd *ta = container(a, fs_fat32_ClusCacheNd, clusCacheNd);
	register u64 tb = *(u64 *)b;
	return ta->off == tb ? 0 : (ta->off < tb ? -1 : 1);
}

__always_inline__ int fs_fat32_FatCacheNd_cmp(RBNode *a, RBNode *b) {
	register fs_fat32_FatCacheNd *ta = container(a, fs_fat32_FatCacheNd, rbNd),
		*tb = container(b, fs_fat32_FatCacheNd, rbNd);
	return ta->off < tb->off;
}

__always_inline__ int fs_fat32_FatCacheNd_match(RBNode *a, void *b) {
	register fs_fat32_FatCacheNd *ta = container(a, fs_fat32_FatCacheNd, rbNd);
	register u32 tb = *(u32 *)b;
	return ta->off == tb ? 0 : (ta->off < tb ? -1 : 1);
}

__always_inline__ int fs_fat32_EntryCache_cmp(RBNode *a, RBNode *b) {
	register fs_fat32_Entry *ta = container(a, fs_fat32_Entry, cacheNd), *tb = container(a, fs_fat32_Entry, cacheNd);
	return strcmp16(ta->vfsEntry.path, tb->vfsEntry.path);
}

__always_inline__ int fs_fat32_EntryCache_match(RBNode *a, void *b) {
	register fs_fat32_Entry *ta = container(a, fs_fat32_Entry, cacheNd);
	return strcmp16(ta->vfsEntry.path, b);
}

RBTree_insertDef(fs_fat32_FatCacheNd_insert, fs_fat32_FatCacheNd_cmp);
RBTree_findDef(fs_fat32_FatCacheNd_find, fs_fat32_FatCacheNd_match);

RBTree_insertDef(fs_fat32_ClusCacheNd_insert, fs_fat32_ClusCacheNd_cmp);
RBTree_findDef(fs_fat32_ClusCacheNd_find, fs_fat32_ClusCacheNd_match);

RBTree_insertDef(fs_fat32_EntryCache_insert, fs_fat32_EntryCache_cmp);
RBTree_findDef(fs_fat32_EntryCache_find, fs_fat32_EntryCache_match)

int fs_fat32_initParCache(fs_fat32_Partition *par) {
	RBTree_init(&par->cache.fatTr, fs_fat32_FatCacheNd_insert, fs_fat32_FatCacheNd_find);
	RBTree_init(&par->cache.clusCacheTr, fs_fat32_ClusCacheNd_insert, fs_fat32_ClusCacheNd_find);
	RBTree_init(&par->cache.entryTr, fs_fat32_EntryCache_insert, fs_fat32_EntryCache_find);

	List_init(&par->cache.freeClusCacheLst);
	List_init(&par->cache.freeEntryLst);
	par->cache.fatNum = 0;
	par->cache.freeClusCacheNum = 0;
	par->cache.freeEntryNum = 0;

    return res_SUCC;
}

int fs_fat32_init() {
	printk(screen_log, "fs_fat32_init()");
	memset(&fs_fat32_drv, 0, sizeof(fs_vfs_Driver));
	memcpy("fat32", fs_fat32_drv.name, sizeof("fat32"));
	fs_fat32_drv.lookup = fs_fat32_lookup;

	fs_fat32_drv.openFile = fs_fat32_openFile;
	fs_fat32_drv.openDir = fs_fat32_openDir;
	
	fs_fat32_drv.closeFile = fs_fat32_closeFile;
	fs_fat32_drv.closeDir = fs_fat32_closeDir;

	fs_fat32_drv.releaseEntry = fs_fat32_releaseEntry;

	fs_fat32_drv.chkGpt = fs_fat32_chkGpt;

	fs_fat32_drv.installGptPar = fs_fat32_installGptPar;

	fs_fat32_drv.getRootEntry = fs_fat32_getRootEntry;

	fs_fat32_fileApi.seek = fs_fat32_FileAPI_seek;
	fs_fat32_fileApi.write = fs_fat32_FileAPI_write;
	fs_fat32_fileApi.read = fs_fat32_FileAPI_read;

	fs_fat32_dirApi.nxt = fs_fat32_DirAPI_nxt;

	fs_vfs_registerDriver(&fs_fat32_drv);
}