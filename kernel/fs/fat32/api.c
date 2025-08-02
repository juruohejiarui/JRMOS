#include <fs/fat32/api.h>
#include <fs/api.h>
#include <lib/string.h>
#include <screen/screen.h>
#include <mm/mm.h>

int fs_fat32_chk(fs_fat32_BootSector *bs) {
	if (bs->bytesPerSec != 512 || bs->bootSigEnd != 0xaa55 || bs->fatSz16 != 0)
		return res_FAIL;
	return res_SUCC;
}

__always_inline__ int _initParInfo(fs_fat32_Partition *par) {
	// read FSInfo sector
	u64 res = par->par.disk->device->read(par->par.disk->device, par->par.st + 1, 1, &par->fsSec);
	if (res != 1) return res_FAIL;

	par->fstDtSec = par->par.st + par->bootSec.rsvdSecCnt + par->bootSec.fatSz32 * par->bootSec.numFats;
	par->fstFat1Sec = par->par.st + par->bootSec.rsvdSecCnt;
	par->fstFat2Sec = par->fstFat1Sec + par->bootSec.fatSz32;
	par->bytesPerClus = par->bootSec.secPerClus * par->bootSec.bytesPerSec;
	par->lbaPerClus = par->bytesPerClus / hw_diskdev_lbaSz;

	printk(screen_log, 
		"fat32: %p: info: fstDtSec:%016lx fstFat1Sec:%016lx fstFat2Sec:%016lx bytesPerClus:%016lx lbaPerClus:%016lx\n",
		par,
		par->fstDtSec, par->fstFat1Sec, par->fstFat2Sec, par->bytesPerClus, par->lbaPerClus);
	
	return res_SUCC;
}

__always_inline__ int fs_fat32_ClusCacheNd_cmp(RBNode *a, RBNode *b) {
	register fs_fat32_ClusCacheNd *ta = container(a, fs_fat32_ClusCacheNd, rbNd),
		*tb = container(b, fs_fat32_ClusCacheNd, rbNd);
	return ta->off < tb->off;
}

__always_inline__ int fs_fat32_ClusCacheNd_match(RBNode *a, void *b) {
	register fs_fat32_ClusCacheNd *ta = container(a, fs_fat32_ClusCacheNd, rbNd);
	register u64 tb = *(u64 *)b;
	return ta->off == tb ? 0 : (ta->off < tb ? -1 : 1);
}

RBTree_insertDef(fs_fat32_ClusCacheNd_insert, fs_fat32_ClusCacheNd_cmp);
RBTree_findDef(fs_fat32_ClusCacheNd_find, fs_fat32_ClusCacheNd_match);

__always_inline__ int _initParCache(fs_fat32_Partition *par) {
	RBTree_init(&par->cache.clusTr, fs_fat32_ClusCacheNd_insert, fs_fat32_ClusCacheNd_find);
	List_init(&par->cache.freeClusLst);
	SpinLock_init(&par->cache.lck);
}

fs_fat32_ClusCacheNd *fs_fat32_getClusCacheNd(fs_fat32_Partition *par, u64 off) {
	SpinLock_lock(&par->cache.lck);
	RBNode *nd = RBTree_find(&par->cache.clusTr, &off);
	fs_fat32_ClusCacheNd *cache;
	// doest not exist this cache, create node
	if (nd == NULL || (cache = container(nd, fs_fat32_ClusCacheNd, rbNd))->off != off) {
		// create cache & read from disk
		void *clus = mm_kmalloc(hw_diskdev_lbaSz * par->lbaPerClus + sizeof(fs_fat32_ClusCacheNd), mm_Attr_Shared, NULL);
		if (clus == NULL || 
			par->par.disk->device->read(par->par.disk->device, par->par.st + off * par->lbaPerClus, par->lbaPerClus, clus) != par->lbaPerClus) {
			if (clus != NULL) mm_kfree(clus, mm_Attr_Shared);
			return NULL;
		} 

		// set up cache information
		cache = (void *)(clus + hw_diskdev_lbaSz * par->lbaPerClus);
		SpinLock_init(&cache->modiLck);
		cache->off = off;
		cache->refCnt = cache->modiCnt = 0;
		// add to cache tree
		RBTree_ins(&par->cache.clusTr, &cache->rbNd);
	}
	// remove it from free list
	if ((cache->refCnt++) == 0) List_del(&cache->freeLstNd);
	SpinLock_unlock(&par->cache.lck);
	return cache;
}

// flush cache and write to disk
int fs_fat32_flushClusCacheNd(fs_fat32_Partition *par, fs_fat32_ClusCacheNd *nd) {
	return par->par.disk->device->write(
			par->par.disk->device, par->par.st + nd->off * par->lbaPerClus, par->lbaPerClus, nd->clus)
		 == par->lbaPerClus ? res_SUCC : res_FAIL;
}

int fs_fat32_releaseClusCacheNd(fs_fat32_Partition *par, fs_fat32_ClusCacheNd *nd) {
	int res = res_SUCC;
	SpinLock_lock(&par->cache.lck);
	if ((--nd->refCnt) == 0) {
		res = fs_fat32_flushClusCacheNd(par, nd);
		List_insTail(&par->cache.freeClusLst, &nd->freeLstNd);
	}
	SpinLock_lock(&par->cache.lck);
	return res;
}
// should be called whenever cache is modified.
int fs_fat32_endModiClusCacheNd(fs_fat32_Partition *par, fs_fat32_ClusCacheNd *nd) {
	SpinLock_lock(&nd->modiLck);
	nd->modiCnt++;
	if (nd->modiCnt > fat32_ClusCacheNd_MaxModiCnt)
		nd->modiCnt -= fat32_ClusCacheNd_MaxModiCnt,
		fs_fat32_flushClusCacheNd(par, nd);
	SpinLock_unlock(&nd->modiLck);
}

int fs_fat32_clrFreeCache(fs_fat32_Partition *par) {
	SpinLock_lock(&par->cache.lck);
	ListNode *node = par->cache.freeClusLst.next;
	while (node != &par->cache.freeClusLst) {
		fs_fat32_ClusCacheNd *nd = container(node, fs_fat32_ClusCacheNd, freeLstNd);
		node = node->next;
		RBTree_del(&par->cache.clusTr, &nd->rbNd);
		mm_kfree(nd, mm_Attr_Shared);
	}
	List_init(&par->cache.freeClusLst);
	SpinLock_unlock(&par->cache.lck);
	return res_SUCC;
}

int fs_fat32_initPar(fs_fat32_Partition *par, fs_fat32_BootSector *bs, fs_Disk *disk, u64 stLba, u64 edLba) {
	memcpy(bs, &par->bootSec, sizeof(fs_fat32_BootSector));
	par->par.st = stLba;
	par->par.ed = edLba;

	// add parition to partition list of disk
	fs_Disk_addPar(disk, &par->par);

	printk(screen_log, 
		"fat32: %p: fs struct:\n"
		"\tleadSig: %08x\n"
		"\tstruSig: %08x\n"
		"\tfreeCnt: %08x\n"
		"\tnxtFree: %08x\n"
		"\ttrailSig:%08x\n",
		par, par->fsSec.leadSig, par->fsSec.structSig, par->fsSec.freeCnt, par->fsSec.nxtFree, par->fsSec.trailSig);
	
	// initialize base information
	if (_initParInfo(par) == res_FAIL) return res_FAIL;

	// initialize cache
	if (_initParCache(par) == res_FAIL) return res_FAIL;

	// launch file system server
	/// @todo

	// enum root directory (for testing)

	return res_SUCC;
}