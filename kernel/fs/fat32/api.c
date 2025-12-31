#include "inner.h"
#include <fs/api.h>
#include <fs/gpt/api.h>
#include <lib/string.h>
#include <screen/screen.h>
#include <mm/mm.h>
#include "inner.h"

__always_inline__ int _isDeletedEnt(void *ent) {
	return *(char *)ent == fs_fat32_DeleteFlag;
}

__always_inline__ void _deleteEnt(void *ent) {
	*(char *)ent = fs_fat32_DeleteFlag;
}

int fs_fat32_chkBootSec(fs_fat32_BootSector *bs) {
	if (bs->bootSigEnd != 0xaa55 || bs->fatSz16 != 0)
		return res_FAIL;
	if (bs->bytesPerSec != 512) {
		printk(screen_err, "fat32: %p: invalid bytesPerSec:%x\n", bs, bs->bytesPerSec);
		return res_FAIL;
	}
	return res_SUCC;
}

int fs_fat32_chkGpt(fs_Disk *disk, fs_gpt_ParEntry *entry) {
	fs_fat32_BootSector *bs = mm_kmalloc(hw_diskdev_lbaSz, mm_Attr_Shared, NULL);
	disk->device->read(disk->device, entry->stLba, 1, bs);
	printk(screen_log, 
		"fs: fat32: disk %p partition %p\n"
		"\t\toemName:     %s\n"
		"\t\tbytesPerSec: %#06x SecPerClus:  %#06x\n"
		"\t\tfatSz16:     %#06x fatSz32:     %#06x\n"
		"\t\tbootSigEnd:  %#06x BytesPerSec: %#06x\n",
		disk, entry, 
		bs->oemName, bs->bytesPerSec, bs->secPerClus,
		bs->fatSz16, bs->fatSz32, bs->bootSigEnd,
		bs->bytesPerSec);
	if (fs_fat32_chkBootSec(bs) != res_SUCC) {
		printk(screen_err, "fs: fat32: disk %p partition %p is not a valid FAT32 partition\n", disk, entry);
		mm_kfree(bs, mm_Attr_Shared);
		return res_FAIL;
	}
	mm_kfree(bs, mm_Attr_Shared);
	printk(screen_succ, "fs: fat32: disk %p partition %p is a valid FAT32 partition\n", disk, entry);
	return res_SUCC;
}

static fs_fat32_Entry *_allocEntry(fs_fat32_Partition *par, fs_fat32_DirEntry *dirEntry) {
	fs_fat32_Entry *entry = mm_kmalloc(sizeof(fs_fat32_Entry), 0, (void *)fs_fat32_drv.releaseEntry);
	memcpy(dirEntry, &entry->dirEntry, sizeof(fs_fat32_DirEntry));

	entry->vfsEntry.par = (void *)&par->par;
	entry->vfsEntry.size = dirEntry->fileSz;

	return entry;
}

static u8 _chksum(fs_fat32_DirEntry *dirEnt) {
	u8 res = 0;
	u8 *ptr = (u8 *)dirEnt;
	for (i16 len = 11; len >= 0; len--) {
		res = ((res & 1) ? 0x80 : 0) + (res >> 1) + *ptr++;
	}
	return res;
}

static int _chkLEnts(fs_fat32_DirEntry *sEnt, fs_fat32_LDirEntry *lEnts, int lEntNum) {
	u8 chksum = _chksum(sEnt);
	
	for (; lEntNum > 0; lEntNum--) if (lEnts[lEntNum].chksum != chksum) return res_FAIL;
	return res_SUCC; 
}

static int _parseSEnt(fs_fat32_DirEntry *sEnt, u16 *buf) {
	int len;
	u8 *buf8 = (void *)buf;
	memcpy(sEnt->name, buf8, (len = 8));
	// convert file name
	if (sEnt->nameCase & fs_fat32_DirEntry_nameCase_LowName)
		for (int i = 0; i < 8; i++) buf8[i] = toLower(buf8[i]);
	else
		for (int i = 0; i < 8; i++) buf8[i] = toUpper(buf8[i]);
	while (*(buf8 + len - 1) == ' ') len--;

	buf8[len++] = '.';

	memcpy(sEnt->name, buf8 + len, 3);
	// convert file ext name
	if (sEnt->nameCase & fs_fat32_DirEntry_nameCase_LowExtName)
		for (int i = 0; i < 3; i++) buf8[i + len] = toLower(buf8[i + len]);
	else
		for (int i = 0; i < 3; i++) buf8[i + len] = toUpper(buf8[i + len]);
	len += 3;
	while (*(buf8 + len - 1) == ' ' || *(buf8 + len - 1) == '.') len--;

	// convert to utf-16
	for (int t = len - 1; t >= 0; t--)
		buf[t] = buf8[t];
	buf[len] = 0;
	return len;
}

static int _parseLEnt(fs_fat32_DirEntry *sEnt, fs_fat32_LDirEntry *lEnts, int lEntNum, u16 *buf) {
	int len = 0;
	for (int i = 1; i <= lEntNum; i++) {
		memcpy(lEnts[i].name1, buf + len, sizeof(lEnts[i].name1));
		len += sizeof(lEnts[i].name1) / sizeof(u16);
		memcpy(lEnts[i].name2, buf + len, sizeof(lEnts[i].name2));
		len += sizeof(lEnts[i].name2) / sizeof(u16);
		memcpy(lEnts[i].name3, buf + len, sizeof(lEnts[i].name3));
		len += sizeof(lEnts[i].name3) / sizeof(u16);
	}
	while (buf[len - 1] == 0 || buf[len - 1] == 0xffff) len--;
}

static int _parseName(fs_fat32_DirEntry *sEnt, fs_fat32_LDirEntry *lEnts, int lEntNum, u16 *buf) {
	if (_chkLEnts(sEnt, lEnts, lEntNum)) return _parseLEnt(sEnt, lEnts, lEntNum, buf);
	else return _parseSEnt(sEnt, buf);
}

// This function should be called when cache->entryTr is locked
// when a cache is found, reference to this entry increase
fs_fat32_Entry *_findEntryCache(fs_fat32_Partition *par, u16 *path) {
	RBNode *nd = RBTree_find(&par->cache.entryTr, path);
	fs_fat32_Entry *entry;
	if (nd && !strcmp16((entry = container(nd, fs_fat32_Entry, cacheNd))->vfsEntry.path, path)) {
		if (entry->ref++ == 0) List_del(&entry->freeCacheNd), par->cache.freeEntryNum--;
		return entry;
	} else return NULL;
}

// This function should be called when cache->entryTr is locked
static void _insNewEntryCache(fs_fat32_Partition *par, fs_fat32_Entry *ent) {
	ent->ref = 1;
	RBTree_ins(&par->cache.entryTr, &ent->cacheNd);
}

// functions for vfs drivers
fs_vfs_Entry *fs_fat32_lookup(fs_vfs_Entry *cur, u16 *name) {
	fs_fat32_Partition *par;

	fs_fat32_DirEntry *sEnt;
	static fs_fat32_LDirEntry lEnts[64];

	fs_fat32_Entry *resEnt;
	u64 curClus;

	u16 buf[fs_vfs_maxNameLen];
	{
		fs_fat32_Entry *fat32Cur = container(cur, fs_fat32_Entry, vfsEntry), *entry;
		par = container(fat32Cur->vfsEntry.par, fs_fat32_Partition, par);
		curClus = fs_fat32_DirEntry_getFstClus(&fat32Cur->dirEntry);
	}
	
	RBTree_lck(&par->cache.entryTr);

	static u16 path[fs_vfs_maxPathLen];
	int pathLen = strlen16(cur->path);
	int nameLen = strlen16(name);

	// get full path
	memcpy(cur->path, path, pathLen * sizeof(u16));
	path[pathLen] = fs_vfs_Separator;
	memcpy(name, path + pathLen + 1, nameLen * sizeof(u16));

	// search entry cache
	resEnt = _findEntryCache(par, path);
	if (resEnt) {
		RBTree_unlck(&par->cache.entryTr);
		return &resEnt->vfsEntry;
	}

	u32 off = 0;
	fs_fat32_ClusCacheNd *clusCache = fs_fat32_getClusCacheNd(par, curClus);

	int lEntNum = 0;

	int bufLen;
	
	// scan clusters
	// example of one entry with 3 long entry:
	// L [ord=0x43, attr=0x0f]
	// L [ord=0x02, attr=0x0f]
	// L [ord=0x01, attr=0x0f]
	// S [attr!=0x0f]
	while (clusCache != NULL) {
		for (off = 0; off < par->bytesPerClus; off += sizeof(fs_fat32_LDirEntry)) {
			fs_fat32_LDirEntry *lEnt = (void *)(clusCache->clus + off);
			if (_isDeletedEnt(lEnt)) continue;
			if (lEnt->attr = fs_fat32_DirEntry_attr_LongName) {
				int ord = lEnt->ord & 0x3f;
				memcpy(lEnt, &lEnts[ord], sizeof(fs_fat32_LDirEntry));
				if (lEnt->ord & 0x40) lEntNum = ord;
			}
			// is a short entry
			else {
				sEnt = (void *)lEnt;
				
				bufLen = _parseName(sEnt, lEnts, lEntNum, buf);

				if (bufLen == nameLen && memcmp(name, buf, bufLen * sizeof(u16)))
					goto lookup_findNode;
			}
		}
		fs_fat32_releaseClusCacheNd(par, clusCache, 0);
		curClus = fs_fat32_getNxtClus(par, curClus);
		if (curClus != fs_fat32_ClusEnd) clusCache = fs_fat32_getClusCacheNd(par, curClus);
		else break;
	}
	return NULL;

	lookup_findNode:
	// make entry
	resEnt = _allocEntry(par, sEnt);
	
	// set path
	memcpy(path, resEnt->vfsEntry.path, (nameLen + pathLen + 1) * sizeof(u16));

	// add to entry cache tree
	_insNewEntryCache(par, resEnt);
	
	RBTree_unlck(&par->cache.entryTr);
	return &resEnt->vfsEntry;
}

fs_vfs_Dir *fs_fat32_openDir(fs_vfs_Entry *ent) {
	fs_fat32_Entry *fat32Ent = container(ent, fs_fat32_Entry, vfsEntry);
	fs_fat32_Partition *par = container(ent->par, fs_fat32_Partition, par);
	if (~fat32Ent->dirEntry.attr & fs_fat32_DirEntry_attr_Dir) {
		printk(screen_err, "fs: fat32: %S in partition %S is not directory.\n", ent->path, par->par.name);
		return NULL;
	}
	RBTree_lck(&par->cache.entryTr);
	fat32Ent->ref++;
	RBTree_unlck(&par->cache.entryTr);

	fs_fat32_Dir *dir = mm_kmalloc(sizeof(fs_fat32_Dir), 0, (void *)fs_fat32_drv.closeDir);
	dir->dir.api = &fs_fat32_dirApi;
	dir->dir.par = (void *)par;
	dir->dir.ent = ent;

	dir->clusId = fs_fat32_DirEntry_getFstClus(&fat32Ent->dirEntry);
	dir->clusOff = 0;
	
	SpinLock_init(&dir->lck);

	return &dir->dir;
}

int fs_fat32_closeDir(fs_vfs_Dir *dir) {
	fs_fat32_Dir *fat32Dir = container(dir, fs_fat32_Dir, dir);
	SpinLock_lock(&fat32Dir->lck);
	fs_fat32_Entry *ent = container(fat32Dir->dir.ent, fs_fat32_Entry, vfsEntry);
	fs_fat32_Partition *par = container(dir->par, fs_fat32_Partition, par);

	return fs_fat32_releaseEntry(&ent->vfsEntry);
}

int fs_fat32_releaseEntry(fs_vfs_Entry *entry) {
	fs_fat32_Entry *fat32Entry = container(entry, fs_fat32_Entry, vfsEntry);
	fs_fat32_Partition *par = container(entry->par, fs_fat32_Partition, par);
	int res = res_SUCC;

	RBTree_lck(&par->cache.entryTr);
	if (--fat32Entry->ref == 0) {
		List_insTail(&par->cache.freeEntryLst, &fat32Entry->freeCacheNd);
		if (par->cache.freeEntryNum == fs_fat32_FreeEntryCache_MaxNum) {
			fat32Entry = container(List_getHead(&par->cache.freeEntryLst), fs_fat32_Entry, freeCacheNd);
			RBTree_del(&par->cache.entryTr, &fat32Entry->cacheNd);
			List_del(&fat32Entry->freeCacheNd);
			res = mm_kfree(fat32Entry, mm_Attr_Shared);
		} else par->cache.freeEntryNum++;
	}
	RBTree_unlck(&par->cache.entryTr);

	return res;
}

fs_vfs_Entry *fs_fat32_getRootEntry(fs_Partition *par) {
	fs_fat32_Partition *fat32Par = container(par, fs_fat32_Partition, par);

	fs_fat32_Entry *root = fat32Par->root;
	root->ref++;

	return &root->vfsEntry;
}

fs_Partition *fs_fat32_installGptPar(fs_Disk *disk, fs_gpt_ParEntry *entry) {
	fs_fat32_Partition *par = mm_kmalloc(sizeof(fs_fat32_Partition), mm_Attr_Shared, NULL);

	void *buf = mm_kmalloc(hw_diskdev_lbaSz, mm_Attr_Shared, NULL);
	disk->device->read(disk->device, entry->stLba, 1, buf);

	par->par.st = entry->stLba;
	par->par.ed = entry->edLba;
	par->par.disk = disk;
	memcpy(buf, &par->bootSec, sizeof(fs_fat32_BootSector));

	mm_kfree(buf, mm_Attr_Shared);

	// initialize base information
	if (fs_fat32_initParInfo(par) == res_FAIL) goto installGptPar_fail;

	// initialize cache
	if (fs_fat32_initParCache(par) == res_FAIL) goto installGptPar_fail;


	// make root
	par->par.drv = &fs_fat32_drv;
	{
		fs_fat32_Entry *root = mm_kmalloc(sizeof(fs_fat32_Entry), mm_Attr_Shared, NULL);
		memset(root, 0, sizeof(fs_fat32_Entry));
		par->root = root;

		root->vfsEntry.par = (void *)&par->par;
		root->vfsEntry.flags = fs_vfs_EntryAttr_flags_IsDir;
		root->ref = 1;

		fs_fat32_DirEntry_setFstClus(&root->dirEntry, par->fstDtSec);

		RBTree_ins(&par->cache.entryTr, &root->cacheNd);
	}
	printk(screen_succ, "fs: fat32: driver is applied to disk %p partition %p\n", disk, entry);

	return &par->par;

	installGptPar_fail:
	mm_kfree(par, mm_Attr_Shared);
	return NULL;
}

fs_vfs_Entry *fs_fat32_DirAPI_nxt(fs_vfs_Dir *dir) {
	fs_fat32_Dir *fat32Dir = container(dir, fs_fat32_Dir, dir);
	
	SpinLock_lock(&fat32Dir->lck);
	fs_fat32_Entry *ent = container(dir->ent, fs_fat32_Entry, vfsEntry), *resEnt = NULL;
	fs_fat32_Partition *par = container(dir->par, fs_fat32_Partition, par);
	fs_fat32_ClusCacheNd *cache;

	if (fat32Dir->clusId == fs_fat32_ClusEnd) return NULL;

	cache = NULL;

	fs_fat32_LDirEntry lEnts[32];
	fs_fat32_DirEntry *sEnt = NULL;
	int lEntNum;

	u16 name[fs_vfs_maxNameLen];
	int nameLen = -1;

	while (fat32Dir->clusId != fs_fat32_ClusEnd) {
		if (cache == NULL) cache = fs_fat32_getClusCacheNd(par, fat32Dir->clusId);

		for (; fat32Dir->clusOff < par->bytesPerClus && nameLen == -1; fat32Dir->clusOff += sizeof(fs_fat32_DirEntry)) {
			fs_fat32_LDirEntry *lEnt = (void *)(cache->clus + fat32Dir->clusOff);
			if (_isDeletedEnt(lEnt)) continue;
			if (lEnt->attr == fs_fat32_DirEntry_attr_LongName) {
				int ord = lEnt->ord & 0x3f;
				memcpy(lEnt, &lEnts[ord], sizeof(fs_fat32_LDirEntry));
				if (lEnt->ord & 0x40) lEntNum = ord; 
			} else {
				// is a short entry
				sEnt = (void *)lEnt;

				nameLen = _parseName(sEnt, lEnts, lEntNum, name);
				continue;
			}
		}
		if (fat32Dir->clusOff == par->bytesPerClus) {
			fs_fat32_releaseClusCacheNd(par, cache, 0);
			cache = NULL;
			fat32Dir->clusOff = 0;
			fat32Dir->clusId = fs_fat32_getNxtClus(par, fat32Dir->clusId);
		}
	}
	SpinLock_unlock(&fat32Dir->lck);

	if (sEnt == NULL) goto DirAPI_nxt_noNxt;

	RBTree_lck(&par->cache.entryTr);
	static u16 path[fs_vfs_maxPathLen];
	u16 pathLen = strlen16(ent->vfsEntry.path);
	memcpy(ent->vfsEntry.path, path, pathLen * sizeof(u16));
	path[pathLen] = fs_vfs_Separator;
	memcpy(name, path + pathLen + 1, nameLen * sizeof(u16));

	if ((resEnt = _findEntryCache(par, path)) == NULL) {
		resEnt = _allocEntry(par, sEnt);

		memcpy(path, resEnt->vfsEntry.path, (pathLen + 1 + nameLen) * sizeof(u16));
		
		_insNewEntryCache(par, resEnt);
	}
	RBTree_unlck(&par->cache.entryTr);

	SpinLock_unlock(&fat32Dir->lck);

	DirAPI_nxt_noNxt:
	if (cache != NULL) fs_fat32_releaseClusCacheNd(par, cache, 0);
	return &resEnt->vfsEntry;
}