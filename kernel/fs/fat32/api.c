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

RBTree_insertDef(fs_fat32_ClusCacheNd_insert, fs_fat32_ClusCacheNd_cmp);
RBTree_findDef(fs_fat32_ClusCacheNd_find, fs_fat32_ClusCacheNd_match);

RBTree_insertDef(fs_fat32_FatCacheNd_insert, fs_fat32_FatCacheNd_cmp);
RBTree_findDef(fs_fat32_FatCacheNd_find, fs_fat32_FatCacheNd_match);

__always_inline__ int _initParCache(fs_fat32_Partition *par) {
	RBTree_init(&par->cache.clusTr, fs_fat32_ClusCacheNd_insert, fs_fat32_ClusCacheNd_find);
	List_init(&par->cache.freeClusLst);
	SpinLock_init(&par->cache.clusLck);

	RBTree_init(&par->cache.fatTr, fs_fat32_FatCacheNd_insert, fs_fat32_FatCacheNd_find);
	par->cache.fatNum = 0;
	SpinLock_init(&par->cache.fatLck);
}

int fs_fat32_readSec(fs_fat32_Partition *par, u64 off, u64 sz, void *buf) {
	register hw_DiskDev *dev = par->par.disk->device;
	if (dev->read(dev, par->par.st + off, sz, buf) != sz) {
		printk(screen_err, "fat32: read sector %016lx failed\n", off);
		return res_FAIL;
	}
	return res_SUCC;
}

int fs_fat32_writeSec(fs_fat32_Partition *par, u64 off, u64 sz, void *buf) {
	register hw_DiskDev *dev = par->par.disk->device;
	if (dev->write(dev, par->par.st + off, sz, buf) != sz) {
		printk(screen_err, "fat32: write sector %016lx failed\n", off);
		return res_FAIL;
	}
	return res_SUCC;
}

fs_fat32_ClusCacheNd *``fs_fat32_getClusCacheNd(fs_fat32_Partition *par, u64 off) {
	SpinLock_lock(&par->cache.clusLck);
	RBNode *nd = RBTree_find(&par->cache.clusTr, &off);
	fs_fat32_ClusCacheNd *cache;
	// doest not exist this cache, create node
	if (nd == NULL || (cache = container(nd, fs_fat32_ClusCacheNd, rbNd))->off != off) {
		printk(screen_log, "fat32: getClusCacheNd: read cluster %016lx\n", off);
		// create cache & read from disk
		void *clus = mm_kmalloc(hw_diskdev_lbaSz * par->lbaPerClus, mm_Attr_Shared, NULL);
		cache = mm_kmalloc(sizeof(fs_fat32_ClusCacheNd), mm_Attr_Shared, NULL);
		if (clus == NULL || cache == NULL) {
			if (clus != NULL) mm_kfree(clus, mm_Attr_Shared);
			if (cache != NULL) mm_kfree(cache, mm_Attr_Shared);
			printk(screen_err, "fat32: getClusCacheNd: failed to allocate memory for cluster cache.\n");
			return NULL;
		}
		if (fs_fat32_readSec(par, off * par->lbaPerClus + par->fstDtSec, par->lbaPerClus, clus) & res_FAIL) {
			printk(screen_err, "fat32: getClusCacheNd: failed to read cluster %016lx.\n", off);
			mm_kfree(clus, mm_Attr_Shared);
			mm_kfree(cache, mm_Attr_Shared);
			SpinLock_unlock(&par->cache.clusLck);
			return NULL;
		}
		// set up cache information
		SpinLock_init(&cache->modiLck);
		cache->off = off;
		cache->refCnt = cache->modiCnt = 0;
		cache->clus = clus;
		// add to cache tree
		RBTree_ins(&par->cache.clusTr, &cache->rbNd);
		List_init(&cache->freeLstNd);
	}
	// remove it from free list
	if ((cache->refCnt++) == 0) List_del(&cache->freeLstNd);
	SpinLock_unlock(&par->cache.clusLck);
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
	SpinLock_lock(&par->cache.clusLck);
	if ((--nd->refCnt) == 0) {
		res |= fs_fat32_flushClusCacheNd(par, nd);
		List_insTail(&par->cache.freeClusLst, &nd->freeLstNd);
	}
	SpinLock_lock(&par->cache.clusLck);
	return res;
}
// should be called whenever cache is modified.
int fs_fat32_endModiClusCacheNd(fs_fat32_Partition *par, fs_fat32_ClusCacheNd *nd) {
	int res = res_SUCC;
	SpinLock_lock(&nd->modiLck);
	nd->modiCnt++;
	if (nd->modiCnt > fs_fat32_ClusCacheNd_MaxModiCnt)
		nd->modiCnt -= fs_fat32_ClusCacheNd_MaxModiCnt,
		res |= fs_fat32_flushClusCacheNd(par, nd);
	SpinLock_unlock(&nd->modiLck);
	return res;
}

int fs_fat32_clrFreeCache(fs_fat32_Partition *par) {
	SpinLock_lock(&par->cache.clusLck);
	ListNode *node = par->cache.freeClusLst.next;
	while (node != &par->cache.freeClusLst) {
		fs_fat32_ClusCacheNd *nd = container(node, fs_fat32_ClusCacheNd, freeLstNd);
		node = node->next;
		RBTree_del(&par->cache.clusTr, &nd->rbNd);
		mm_kfree(nd, mm_Attr_Shared);
	}
	List_init(&par->cache.freeClusLst);
	SpinLock_unlock(&par->cache.clusLck);
	return res_SUCC;
}

u32 fs_fat32_getNxtClus(fs_fat32_Partition *par, u32 clus) {
	// read FAT1
	u64 off = par->fstFat1Sec + (clus * 4) / hw_diskdev_lbaSz;
	u64 idx = (clus * 4) % hw_diskdev_lbaSz / 4;
	SpinLock_lock(&par->cache.fatLck);
	// read fat1 sector
	RBNode *nd = RBTree_find(&par->cache.fatTr, &off);
	fs_fat32_FatCacheNd *cache;
	if (nd == NULL || (cache = container(nd, fs_fat32_FatCacheNd, rbNd))->off != off) {
		printk(screen_log, "fat32: getNxtClus: read fat1 sector %016lx\n", off);
		// remove caches from tree if necessary
		if (par->cache.fatNum > fs_fat32_FatCache_MaxNum) {
			// remove the root
			cache = container(par->cache.fatTr.root, fs_fat32_FatCacheNd, rbNd);
			RBTree_del(&par->cache.fatTr, &cache->rbNd);
			// write to disk
			if (fs_fat32_writeSec(par, cache->off, 1, cache->sec) != res_SUCC)
				goto getNxtClus_fail;
			// reuse the cache
		} else {
			par->cache.fatNum++;
			// create cache & read from disk
			void *sec = mm_kmalloc(hw_diskdev_lbaSz, mm_Attr_Shared, NULL);
			cache = mm_kmalloc(sizeof(fs_fat32_FatCacheNd), mm_Attr_Shared, NULL);
			if (sec == NULL || cache == NULL) {
				if (sec != NULL) mm_kfree(sec, mm_Attr_Shared);
				if (cache != NULL) mm_kfree(cache, mm_Attr_Shared);
				printk(screen_err, "fat32: getNxtClus: failed to allocate memory for fat cache.\n");
				goto getNxtClus_fail;
			}
			cache->sec = sec;
		}
		if (fs_fat32_readSec(par, off, 1, cache->sec) != res_SUCC)
			goto getNxtClus_fail;
		// add to cache tree
		cache->off = off;
		RBTree_ins(&par->cache.fatTr, &cache->rbNd);
	}
	u32 res = cache->sec[off / 4];
	SpinLock_unlock(&par->cache.fatLck);
	return res;
	getNxtClus_fail:
	SpinLock_unlock(&par->cache.fatLck);
	printk(screen_err, "fat32: getNxtClus: clus:%08x -> nxtClus:%08x\n", clus, res);
	return ~0x0u;
}

void print_short_name(fs_fat32_DirEntry *ent) {
	char name[12];
	memcpy(ent->name, name, 8);
	name[8] = '.';
	memcpy(ent->name + 8, name + 9, 3);
	name[12] = '\0';
	for (int i = 10; i >= 0; i--) {
		if (name[i] == ' ' || name[i] == '.') name[i] == '\0';
		else break;
	}
	printk(screen_log, "fat32: short name: %s, attr: %02x, crtDate: %04x, fstClusHi: %04x, fstClusLo: %04x\n",
		name, ent->attr, ent->crtDate, ent->fstClusHi, ent->fstClusLo);
}

void _printRootDir(fs_fat32_Partition *par) {
	u32 curClus = par->bootSec.rootClus;
	while (curClus < 0x0ffffff8) {
		// read root directory
		fs_fat32_ClusCacheNd *cache = fs_fat32_getClusCacheNd(par, curClus);
		if (cache == NULL) {
			printk(screen_err, "fat32: _printRootDir: failed to get cache for cluster %08x\n", curClus);
			return;
		}
		u8 *dt = cache->clus;
		for (int i = 0; i < par->bytesPerClus / sizeof(fs_fat32_LDirEntry); i++) {
			fs_fat32_DirEntry *ent = (fs_fat32_DirEntry *)(dt + i * sizeof(fs_fat32_DirEntry));
			if (ent->name[0] == 0xe5) continue;
			else if (ent->attr == fs_fat32_DirEntry_attr_LongName) {
				printk(screen_log, "fat32: root dir entry: long name: %s\n", ent->name);
				continue;
			}
			print_short_name(ent);
		}
		fs_fat32_releaseClusCacheNd(par, cache);
		curClus = fs_fat32_getNxtClus(par, curClus);
	}
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

	_printRootDir(par);

	return res_SUCC;
}