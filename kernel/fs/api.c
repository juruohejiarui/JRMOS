#include <fs/api.h>
#include <fs/fat32/api.h>
#include <fs/vfs/api.h>
#include <screen/screen.h>

void fs_init() {
	printk(screen_log, "fs_init()\n");
	SafeList_init(&fs_diskLst);
	fs_vfs_init();
	fs_fat32_init();
}

void fs_Partition_init(fs_Partition *par, u64 st, u64 ed, u64 attr, fs_Disk *disk) {
	par->st = st;
	par->ed = ed;
	par->attr = attr;
	par->disk = disk;

	par->status.value = 0;
	SafeList_init(&par->fileLst);
	SafeList_init(&par->dirLst);

	SafeList_insTail(&disk->parLst, &par->diskParLstNd);
}

void fs_Disk_init(fs_Disk *disk, u64 attr) {
	SafeList_init(&disk->parLst);
	SafeList_insTail(&fs_diskLst, &disk->diskLstNd);
	disk->status.value = 0;
	disk->attr = attr;
}

void fs_Disk_addPar(fs_Disk *disk, fs_Partition *par) {
	par->disk = disk;
	SafeList_insTail(&disk->parLst, &par->diskParLstNd);
}