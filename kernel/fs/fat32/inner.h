#ifndef __FS_FAT32_INNER_H__
#define __FS_FAT32_INNER_H__

#include <fs/fat32/desc.h>
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

int fs_fat32_readSec(fs_fat32_Partition *par, u64 off, u64 sz, void *buf);
int fs_fat32_writeSec(fs_fat32_Partition *par, u64 off, u64 sz, void *buf);

fs_fat32_ClusCacheNd *fs_fat32_getClusCacheNd(fs_fat32_Partition *par, u64 off);

// flush cache and write to disk
__always_inline__ int fs_fat32_flushClusCacheNd(fs_fat32_Partition *par, fs_fat32_ClusCacheNd *nd) {
	return fs_fat32_writeSec(par, (nd->off - 2) * par->lbaPerClus + par->fstDtSec, par->lbaPerClus, nd->clus);
}

int fs_fat32_releaseClusCacheNd(fs_fat32_Partition *par, fs_fat32_ClusCacheNd *nd);

int fs_fat32_endModiClusCacheNd(fs_fat32_Partition *par, fs_fat32_ClusCacheNd *nd);

int fs_fat32_clrFreeCache(fs_fat32_Partition *par);

u32 fs_fat32_getNxtClus(fs_fat32_Partition *par, u32 clus);

int fs_fat32_setNxtClus(fs_fat32_Partition *par, u32 clus, u32 nxt);

u32 fs_fat32_allocClus(fs_fat32_Partition *par);

int fs_fat32_initParInfo(fs_fat32_Partition *par);

int fs_fat32_initParCache(fs_fat32_Partition *par);


#endif