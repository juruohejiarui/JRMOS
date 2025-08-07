#include <fs/fat32/api.h>
#include <fs/api.h>
#include <lib/string.h>
#include <screen/screen.h>
#include <mm/mm.h>
#include "inner.h"

int fs_fat32_chk(fs_fat32_BootSector *bs) {
	if (bs->bootSigEnd != 0xaa55 || bs->fatSz16 != 0)
		return res_FAIL;
	if (bs->bytesPerSec != 512) {
		printk(screen_err, "fat32: %p: invalid bytesPerSec:%x\n", bs, bs->bytesPerSec);
		return res_FAIL;
	}
	return res_SUCC;
}



int fs_fat32_initPar(fs_fat32_Partition *par, fs_fat32_BootSector *bs, fs_Disk *disk, u64 stLba, u64 edLba) {
	memcpy(bs, &par->bootSec, sizeof(fs_fat32_BootSector));
	par->par.st = stLba;
	par->par.ed = edLba;

	// add parition to partition list of disk
	fs_Disk_addPar(disk, &par->par);

	// initialize base information
	if (fs_fat32_initParInfo(par) == res_FAIL) return res_FAIL;

	// initialize cache
	if (fs_fat32_initParCache(par) == res_FAIL) return res_FAIL;

	// launch file system server
	/// @todo
	return res_SUCC;
}