#include <fs/fat32/api.h>
#include <lib/string.h>

int fs_fat32_chk(fs_fat32_BootSector *bs) {
	if (bs->bytesPerSec != 512 || bs->bootSigEnd != 0xaa55 || bs->fatSz16 != 0)
		return res_FAIL;
	return res_SUCC;
}

int fs_fat32_initPar(fs_fat32_Partition *par, fs_fat32_BootSector *bs, fs_Disk *disk, u64 stLba, u64 edLba) {
	memcpy(&par->bootSec, bs, sizeof(fs_fat32_BootSector));
	par->par.st = stLba;
	par->par.ed = edLba;
	par->par.disk = disk;

	// add parition to partition list of disk 
}