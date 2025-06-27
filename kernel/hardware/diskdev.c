#include <hardware/diskdev.h>
#include <screen/screen.h>
#include <fs/gpt/api.h>
#include <mm/mm.h>

SafeList hw_diskdev_enbLst, hw_diskdev_unenbLst;

void hw_DiskDev_initMgr() {
	SafeList_init(&hw_diskdev_enbLst);
	SafeList_init(&hw_diskdev_unenbLst);
}

void hw_DiskDev_init(hw_DiskDev *dev,
	u64 (*size)(hw_DiskDev *dev),
	u64 (*read)(hw_DiskDev *dev, u64 off, u64 size, void *buf),
	u64 (*write)(struct hw_DiskDev *dev, u64 off, u64 size, void *buf)) {

	dev->size = size;
	dev->read = read;
	dev->write = write;
}

void hw_DiskDev_register(hw_DiskDev *dev) {
	SafeList_insTail(&hw_diskdev_unenbLst, &dev->lst);
}

int hw_DiskDev_install(hw_DiskDev *dev) {
	int res = dev->device.drv->install(&dev->device);
	if (res == res_SUCC) {
		SafeList_del(&hw_diskdev_unenbLst, &dev->lst);
		SafeList_insTail(&hw_diskdev_enbLst, &dev->lst);
		printk(GREEN, BLACK, "hw: disk: succ to install device %p, size=%ld\n", dev, dev->size(dev));
	}

	fs_gpt_scan(dev);
	return res;
}

int hw_DiskDev_uninstall(hw_DiskDev *dev) {
	int res = dev->device.drv->uninstall(&dev->device);
	if (res == res_SUCC) {
		SafeList_del(&hw_diskdev_enbLst, &dev->lst);
		SafeList_insTail(&hw_diskdev_unenbLst, &dev->lst);
	}
	return res;
}