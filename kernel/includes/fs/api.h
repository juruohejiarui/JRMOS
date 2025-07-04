#ifndef __FS_API_H__
#define __FS_API_H__

#include <fs/desc.h>

__always_inline__ u64 fs_Partition_sz(fs_Partition *par) { return par->ed - par->st + 1; }
__always_inline__ u64 fs_Partition_read(fs_Partition *par, u64 off, u64 sz, void *buf) {
	return par->st + off <= par->ed ? par->disk->device->read(par->disk->device, par->st + off, sz, buf) : 0;
}
__always_inline__ u64 fs_Partition_write(fs_Partition *par, u64 off, u64 sz, void *buf) {
	return par->st + off <= par->ed ? par->disk->device->write(par->disk->device, par->st + off, sz, buf) : 0;
}

void fs_init();

void fs_Partition_init(fs_Partition *par, u64 st, u64 ed, u64 attr, fs_Disk *disk);

void fs_Disk_init(fs_Disk *disk, u64 attr);

int fs_Disk_install(fs_Disk *disk);

int fs_Disk_uninstall(fs_Disk *disk);

#endif