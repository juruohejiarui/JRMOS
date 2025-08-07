#include "inner.h"
#include <screen/screen.h>
#include <mm/mm.h>

int fs_fat32_readSec(fs_fat32_Partition *par, u64 off, u64 sz, void *buf) {
	register hw_DiskDev *dev = par->par.disk->device;
	if (dev->read(dev, par->par.st + off, sz, buf) != sz) {
		printk(screen_err, "fat32: read sector %016lx failed\n", off);
		return res_FAIL;
	}
	return res_SUCC;
}

int fs_fat32_writeSec(fs_fat32_Partition *par, u64 off, u64 sz, void *buf) {
	printk(screen_log, "fat32: write sector %017lx\n", off);
	register hw_DiskDev *dev = par->par.disk->device;
	if (dev->write(dev, par->par.st + off, sz, buf) != sz) {
		printk(screen_err, "fat32: write sector %016lx failed\n", off);
		return res_FAIL;
	}
	return res_SUCC;
}

fs_fat32_ClusCacheNd *fs_fat32_getClusCacheNd(fs_fat32_Partition *par, u64 off) {
	SpinLock_lock(&par->cache.clusLck);
	RBNode *nd = RBTree_find(&par->cache.clusTr, &off);
	fs_fat32_ClusCacheNd *cache;
	// doest not exist this cache, create node
	if (nd == NULL || (cache = container(nd, fs_fat32_ClusCacheNd, rbNd))->off != off) {
		// create cache & read from disk
		void *clus = mm_kmalloc(hw_diskdev_lbaSz * par->lbaPerClus, mm_Attr_Shared, NULL);
		cache = mm_kmalloc(sizeof(fs_fat32_ClusCacheNd), mm_Attr_Shared, NULL);
		if (clus == NULL || cache == NULL) {
			if (clus != NULL) mm_kfree(clus, mm_Attr_Shared);
			if (cache != NULL) mm_kfree(cache, mm_Attr_Shared);
			printk(screen_err, "fat32: getClusCacheNd: failed to allocate memory for cluster cache.\n");
			return NULL;
		}
		if (fs_fat32_readSec(par, (off - 2) * par->lbaPerClus + par->fstDtSec, par->lbaPerClus, clus) & res_FAIL) {
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
	} else {
		// remove it from free list
		if ((cache->refCnt++) == 0) List_del(&cache->freeLstNd);
	}
	
	SpinLock_unlock(&par->cache.clusLck);
	return cache;
}

int fs_fat32_releaseClusCacheNd(fs_fat32_Partition *par, fs_fat32_ClusCacheNd *nd) {
	int res = res_SUCC;
	SpinLock_lock(&par->cache.clusLck);
	if ((--nd->refCnt) == 0) {
		res |= fs_fat32_flushClusCacheNd(par, nd);
		List_insTail(&par->cache.freeClusLst, &nd->freeLstNd);
	}
	SpinLock_unlock(&par->cache.clusLck);
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
	int res = res_SUCC;
	SpinLock_lock(&par->cache.clusLck);
	ListNode *node = par->cache.freeClusLst.next;
	while (node != &par->cache.freeClusLst) {
		fs_fat32_ClusCacheNd *nd = container(node, fs_fat32_ClusCacheNd, freeLstNd);
		node = node->next;
		RBTree_del(&par->cache.clusTr, &nd->rbNd);
		res |= mm_kfree(nd, mm_Attr_Shared);
	}
	List_init(&par->cache.freeClusLst);
	SpinLock_unlock(&par->cache.clusLck);
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
	SpinLock_lock(&par->cache.fatLck);
	// read fat1 sector
	fs_fat32_FatCacheNd *cache = _crtFatCache(par, off);
	if (cache == NULL) goto getNxtClus_fail;
	u32 res = cache->sec[idx];
	SpinLock_unlock(&par->cache.fatLck);
	return res;
	getNxtClus_fail:
	SpinLock_unlock(&par->cache.fatLck);
	return ~0x0u;
}

int fs_fat32_setNxtClus(fs_fat32_Partition *par, u32 clus, u32 nxt) {
    u64 off = (clus * sizeof(u32)) / hw_diskdev_lbaSz;
	u64 idx = (clus * sizeof(u32)) % hw_diskdev_lbaSz /  sizeof(u32);

    SpinLock_lock(&par->cache.fatLck);
	
	// read fat1 sector
	fs_fat32_FatCacheNd *cache = _crtFatCache(par, off);
	if (cache == NULL) goto setNxtClus_fail;
	cache->sec[idx / 4] = nxt;
	SpinLock_unlock(&par->cache.fatLck);
	return res_SUCC;
	setNxtClus_fail:
	SpinLock_unlock(&par->cache.fatLck);
	return res_FAIL | res_DAMAGED;
}

u32 fs_fat32_allocClus(fs_fat32_Partition *par) {
	SpinLock_lock(&par->cache.fatLck);
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
	SpinLock_unlock(&par->cache.fatLck);
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