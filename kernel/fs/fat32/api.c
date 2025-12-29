#include "inner.h"
#include <fs/api.h>
#include <fs/gpt/api.h>
#include <lib/string.h>
#include <screen/screen.h>
#include <mm/mm.h>
#include "inner.h"

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

static fs_fat32_Entry *_allocEntry(fs_fat32_Partition *par, fs_fat32_DirEntry *dirEntry, fs_fat32_LDirEntry *lDirEntry) {
	fs_fat32_Entry *entry = mm_kmalloc(sizeof(fs_fat32_Entry), 0, (void *)fs_fat32_drv.releaseEntry);
	memcpy(dirEntry, &entry->dirEntry, sizeof(fs_fat32_DirEntry));
	memcpy(lDirEntry, &entry->lDirEntry, sizeof(fs_fat32_LDirEntry));

	entry->vfsEntry.par = &par->par;
	entry->vfsEntry.size = dirEntry->fileSz;

	return entry;
}

// functions for vfs drivers
fs_vfs_Entry *fs_fat32_lookup(fs_vfs_Entry *cur, char *name) {
	fs_fat32_Entry *fat32Cur = container(cur, fs_fat32_Entry, vfsEntry);
	fs_fat32_Partition *par = container(fat32Cur->vfsEntry.par, fs_fat32_Partition, par);
	u64 curClus = fs_fat32_DirEntry_getFstClus(&fat32Cur->dirEntry);
	u32 off = 0;
	fs_fat32_ClusCacheNd *clusCache = fs_fat32_getClusCacheNd(par, curClus);

	fs_fat32_LDirEntry *lEntries[32];
	int lEntryCnt = 0;
	  
	while (clusCache != NULL) {
		for (off = 0; off < hw_diskdev_lbaSz; off += sizeof(fs_fat32_LDirEntry)) {
			fs_fat32_LDirEntry *lEnt = (void *)(clusCache->clus + off);
			if (lEnt->ord == 0xe5) continue;
		}
		
	}
	return NULL;
}

int fs_fat32_Dir_nxt(fs_vfs_Dir *dir) {

}

int fs_fat32_releaseEntry(fs_vfs_Entry *entry) {
	// do not thing
	fs_fat32_Entry *fat32Entry = container(entry, fs_fat32_Entry, vfsEntry);
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
		par->par.root = &root->vfsEntry;

		root->vfsEntry.par = &par->par;
		root->vfsEntry.flags = fs_vfs_EntryAttr_flags_IsDir;
		memcpy("/", root->vfsEntry.path, sizeof("/"));

		fs_fat32_DirEntry_setFstClus(&root->dirEntry, par->fstDtSec);
	}
	printk(screen_succ, "fs: fat32: driver is applied to disk %p partition %p\n", disk, entry);

	return &par->par;

	installGptPar_fail:
	mm_kfree(par, mm_Attr_Shared);
	return NULL;
}

