#include <fs/api.h>

void fs_Partition_init(fs_Partition *par, u64 st, u64 ed, u64 attr, fs_Disk *disk) {
	par->st = st;
	par->ed = ed;
	par->attr = attr;
	par->disk = disk;

	// install this partition
	par->status.value = par->openCnt.value = 0;
	SafeList_insTail(&disk->partLst, &par->lst);
}

void fs_Disk_init(fs_Disk *disk, u64 attr) {
	SafeList_init(&disk->partLst);
	disk->status.value = 0;
	disk->attr = attr;
}
