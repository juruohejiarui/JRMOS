#include <fs/fat32/api.h>
#include <fs/api.h>
#include <lib/string.h>
#include <screen/screen.h>

int fs_fat32_chk(fs_fat32_BootSector *bs) {
	if (bs->bytesPerSec != 512 || bs->bootSigEnd != 0xaa55 || bs->fatSz16 != 0)
		return res_FAIL;
	return res_SUCC;
}

int fs_fat32_initPar(fs_fat32_Partition *par, fs_fat32_BootSector *bs, fs_Disk *disk, u64 stLba, u64 edLba) {
	memcpy(bs, &par->bootSec, sizeof(fs_fat32_BootSector));
	par->par.st = stLba;
	par->par.ed = edLba;

	// add parition to partition list of disk
	fs_Disk_addPar(disk, &par->par);

	// read FSInfo sector
	par->par.disk->device->read(par->par.disk->device, par->par.st + 1, 1, &par->fsSec);

	printk(screen_log, 
		"fat32: %p: fs struct:\n"
		"\tleadSig: %08x\n"
		"\tstruSig: %08x\n"
		"\tfreeCnt: %08x\n"
		"\tnxtFree: %08x\n"
		"\ttrailSig:%08x\n",
		par, par->fsSec.leadSig, par->fsSec.structSig, par->fsSec.freeCnt, par->fsSec.nxtFree, par->fsSec.trailSig);
	
	par->fstDtSec = par->par.st + par->bootSec.rsvdSecCnt + par->bootSec.fatSz32 * par->bootSec.numFats;
	par->fstFat1Sec = par->par.st + par->bootSec.rsvdSecCnt;
	par->fstFat2Sec = par->fstFat1Sec + par->bootSec.fatSz32;
	par->bytesPerClus = par->bootSec.secPerClus * par->bootSec.bytesPerSec;

	printk(screen_log, 
		"fat32: %p: info: fstDtSec:%016lx fstFat1Sec:%016lx fstFat2Sec:%016lx bytesPerClus:%016lx\n",
		par,
		par->fstDtSec, par->fstFat1Sec, par->fstFat2Sec, par->bytesPerClus);

	// launch file system server
	

	return res_SUCC;
}