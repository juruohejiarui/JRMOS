#include <fs/fat32/api.h>
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

int fs_fat32_initPar(fs_fat32_Partition *par, fs_fat32_BootSector *bs, fs_Disk *disk, u64 stLba, u64 edLba) {

	// launch file system server
	/// @todo
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
	return res_SUCC;
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

	return &par->par;

	installGptPar_fail:
	mm_kfree(par, mm_Attr_Shared);
	return NULL;
}