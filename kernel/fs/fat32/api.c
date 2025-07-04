#include <fs/fat32/api.h>

int fs_fat32_chk(fs_fat32_BootSector *bs) {
	if (bs->bytesPerSec != 512 || bs->bootSigEnd != 0xaa55 || bs->fatSz16 != 0)
		return res_FAIL;
	return res_SUCC;
}