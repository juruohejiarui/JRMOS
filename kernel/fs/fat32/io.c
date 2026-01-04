#include "inner.h"
#include <screen/screen.h>
#include <mm/mm.h>
#include <lib/string.h>

int fs_fat32_readSec(fs_fat32_Partition *par, u64 off, u64 sz, void *buf) {
	register hw_DiskDev *dev = par->par.disk->device;
	// printk(screen_log, "fs: fat32: read from disk %p, off:%016lx sz:%016lx\n", par->par.disk, off, sz);
	if (dev->read(dev, par->par.st + off, sz, buf) != sz) {
		printk(screen_err, "fat32: read sector %016lx failed\n", off);
		return res_FAIL;
	}
	return res_SUCC;
}

int fs_fat32_writeSec(fs_fat32_Partition *par, u64 off, u64 sz, void *buf) {
	// printk(screen_log, "fat32: write sector %016lx\n", off);
	register hw_DiskDev *dev = par->par.disk->device;
	if (dev->write(dev, par->par.st + off, sz, buf) != sz) {
		printk(screen_err, "fat32: write sector %016lx failed\n", off);
		return res_FAIL;
	}
	return res_SUCC;
}

fs_fat32_ClusCacheNd *fs_fat32_getClusCacheNd(fs_fat32_Partition *par, u64 off) {
	fs_fat32_ClusCacheNd *cacheNd;
	// printk(screen_log, "fs: fat32: search clus cache %016lx\n", off);
	if (off == fs_fat32_ClusEnd) return NULL;
	RBTree_lck(&par->cache.clusCacheTr);
	// printk(screen_succ, "fs: fat32: get lock of clus cache tr.\n");
	RBNode *tmp = RBTree_find(&par->cache.clusCacheTr, &off);
	if (tmp && (cacheNd = container(tmp, fs_fat32_ClusCacheNd, clusCacheNd))->off == off) {
		if (!List_isEmpty(&cacheNd->freeClusCacheNd)) 
			List_del(&cacheNd->freeClusCacheNd),
			List_init(&cacheNd->freeClusCacheNd);
	} else {
		// printk(screen_log, "fs: fat32: no cache\n");
		cacheNd = mm_kmalloc(sizeof(fs_fat32_ClusCacheNd), mm_Attr_Shared, NULL);
		memset(cacheNd, 0, sizeof(fs_fat32_ClusCacheNd));
		void *clus = mm_kmalloc(par->bytesPerClus, mm_Attr_Shared, NULL);
		if (clus == NULL || cacheNd == NULL) {
			if (clus != NULL) mm_kfree(clus, mm_Attr_Shared);
			if (cacheNd != NULL) mm_kfree(cacheNd, mm_Attr_Shared);
			RBTree_unlck(&par->cache.clusCacheTr);
			printk(screen_err, "fat32: getClusCacheNd: failed to allocate memory for cluster cache.\n");
			return NULL;
		}
		cacheNd->off = off;
		cacheNd->clus = clus;
		SpinLock_init(&cacheNd->useLck);
		// printk(screen_log, "fs: fat32: new clus cache %p->%p\n", cacheNd, clus);
		// read from disk
		if (fs_fat32_readSec(par, (off - 2) * par->lbaPerClus + par->fstDtSec, par->lbaPerClus, clus) & res_FAIL) {
			mm_kfree(clus, mm_Attr_Shared);
			mm_kfree(cacheNd, mm_Attr_Shared);
			RBTree_unlck(&par->cache.clusCacheTr);
			printk(screen_err, "fat32: getClusCacheNd: failed to read cluster %016lx.\n", off);
			return NULL;
		}
		List_init(&cacheNd->freeClusCacheNd);
		RBTree_ins(&par->cache.clusCacheTr, &cacheNd->clusCacheNd);
	}
	Atomic_inc(&cacheNd->waitCnt);
	// printk(screen_log, "fs: fat32: get cache %p\n", cacheNd);
	RBTree_unlck(&par->cache.clusCacheTr);
	SpinLock_lock(&cacheNd->useLck);
	Atomic_dec(&cacheNd->waitCnt);
	// printk(screen_log, "fs: fat32: occupy cache %p\n", cacheNd);
	return cacheNd;
}


// should be called whenever cache is modified.
int fs_fat32_releaseClusCacheNd(fs_fat32_Partition *par, fs_fat32_ClusCacheNd *nd, int modi) {
	RBTree_lck(&par->cache.clusCacheTr);
	int free = (nd->waitCnt.value == 0);
	int res = res_SUCC;
	nd->modiCnt += (modi);
	if (nd->modiCnt >= fs_fat32_ClusCacheNd_MaxModiCnt) {
		nd->modiCnt -= fs_fat32_ClusCacheNd_MaxModiCnt;
		res = fs_fat32_flushClusCacheNd(par, nd);
	}
	SpinLock_unlock(&nd->useLck);
	if (free) {
		List_insTail(&par->cache.freeClusCacheLst, &nd->freeClusCacheNd);
		if (par->cache.freeClusCacheNum >= fs_fat32_FreeClusCache_MaxNum) {
			fs_fat32_ClusCacheNd *freeNd = container(par->cache.freeClusCacheLst.next, fs_fat32_ClusCacheNd, freeClusCacheNd);
			List_del(&freeNd->freeClusCacheNd);			
			fs_fat32_flushClusCacheNd(par, nd);
			mm_kfree(&freeNd->clus, mm_Attr_Shared);
			mm_kfree(&freeNd, mm_Attr_Shared);
		} else
			par->cache.freeClusCacheNum++;
	}
	RBTree_unlck(&par->cache.clusCacheTr);
	return res;
}

__always_inline__ fs_fat32_FatCacheNd *_crtFatCache(fs_fat32_Partition *par, u32 off) {
	RBNode *nd = RBTree_find(&par->cache.fatTr, &off);
	fs_fat32_FatCacheNd *cache;
	if (nd == NULL || (cache = container(nd, fs_fat32_FatCacheNd, rbNd))->off != off) {
		// remove caches from tree if necessary
		if (par->cache.fatNum > fs_fat32_FatCache_MaxNum) {
			// remove the root
			cache = container(par->cache.fatTr.root, fs_fat32_FatCacheNd, rbNd);
			RBTree_del(&par->cache.fatTr, &cache->rbNd);
			// write to disk
			if (fs_fat32_writeSec(par, cache->off + par->fstFat1Sec, 1, cache->sec) != res_SUCC)
				return NULL;
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
				return NULL;
			}
			cache->sec = sec;
		}
		if (fs_fat32_readSec(par, off + par->fstFat1Sec, 1, cache->sec) != res_SUCC)
			return NULL;
		// add to cache tree
		cache->off = off;
		RBTree_ins(&par->cache.fatTr, &cache->rbNd);
	}
	return cache;
}

u32 fs_fat32_getNxtClus(fs_fat32_Partition *par, u32 clus) {
	// read FAT1
	u64 off = (clus * sizeof(u32)) / hw_diskdev_lbaSz;
	u64 idx = (clus * sizeof(u32)) % hw_diskdev_lbaSz / sizeof(u32);
	RBTree_lck(&par->cache.fatTr);
	// read fat1 sector
	fs_fat32_FatCacheNd *cache = _crtFatCache(par, off);
	if (cache == NULL) goto getNxtClus_fail;
	u32 res = cache->sec[idx];
	RBTree_unlck(&par->cache.fatTr);
	return res & 0xfffffff8 ? fs_fat32_ClusEnd : res;
	getNxtClus_fail:
	RBTree_unlck(&par->cache.fatTr);
	return fs_fat32_ClusEnd;
}

int fs_fat32_setNxtClus(fs_fat32_Partition *par, u32 clus, u32 nxt) {
    u64 off = (clus * sizeof(u32)) / hw_diskdev_lbaSz;
	u64 idx = (clus * sizeof(u32)) % hw_diskdev_lbaSz / sizeof(u32);

    RBTree_lck(&par->cache.fatTr);
	
	// read fat1 sector
	fs_fat32_FatCacheNd *cache = _crtFatCache(par, off);
	if (cache == NULL) goto setNxtClus_fail;
	cache->sec[idx / 4] = nxt;
	RBTree_unlck(&par->cache.fatTr);
	return res_SUCC;
	setNxtClus_fail:
	RBTree_unlck(&par->cache.fatTr);
	return res_FAIL | res_DAMAGED;
}

u32 fs_fat32_allocClus(fs_fat32_Partition *par) {
	RBTree_lck(&par->cache.fatTr);
	u32 res = 0;
	int i;
	// first, use cache
	RBNode *cur = par->cache.fatTr.left;
	fs_fat32_FatCacheNd *cache;
	while (cur != NULL) {
		cache = container(cur, fs_fat32_FatCacheNd, rbNd);
		for (i = 0; i < hw_diskdev_lbaSz / sizeof(u32); i++)
			if (!cache->sec[i]) {
				res = cache->off * (hw_diskdev_lbaSz / sizeof(u32)) + i;
				goto allocClus_FindInCache;
			}
		cur = RBTree_getNext(&par->cache.fatTr, cur);
	}
	goto allocClus_Scan;
	allocClus_FindInCache:
	cache->sec[i] = 0x0ffffff8;
	RBTree_unlck(&par->cache.fatTr);
	goto allocClus_End;
	allocClus_Scan:
	// otherwise, scan the whole fat
	u32 *lba = mm_kmalloc(hw_diskdev_lbaSz, mm_Attr_Shared, NULL);
	for (u32 off = 0; off < par->bootSec.fatSz32; off++) {
		fs_fat32_readSec(par, par->fstFat1Sec + off, 1, lba);
		for (i = 0; i < hw_diskdev_lbaSz / sizeof(u32); i++)
			if (!lba[i]) {
				res = off * (hw_diskdev_lbaSz / sizeof(u32)) + i;
				cache = _crtFatCache(par, off);
				cache->sec[i] = 0x0fffffff8;
				mm_kfree(lba, mm_Attr_Shared);
				goto allocClus_End;
			}
	}
	mm_kfree(lba, mm_Attr_Shared);
	allocClus_End:
	if (!res) printk(screen_err, "fat32: %p: failed to allocate cluster.\n", par);
	return res;
}