#ifndef __HARDWARE_DISKDEV_H__
#define __HARDWARE_DISKDEV_H__

#include <lib/dtypes.h>
#include <hardware/mgr.h>

#define hw_diskdev_lbaSz 512

typedef struct hw_DiskDev {
	hw_Device device;

	ListNode lst;

	u64 (*size)(struct hw_DiskDev *dev);

	// read to buf and return how many bytes has been written to buffer
	u64 (*read)(struct hw_DiskDev *dev, u64 off, u64 size, void *buf);
	// write to device and return hwo many bytes has been written to buffer
	u64 (*write)(struct hw_DiskDev *dev, u64 off, u64 size, void *buf);
} hw_DiskDev;

// list of enabled(installed) disk device
extern SafeList hw_diskdev_enbLst, hw_diskdev_unenbLst;

void hw_DiskDev_initMgr();

// this function will not initialize the member "device"
void hw_DiskDev_init(hw_DiskDev *dev,
	u64 (*size)(hw_DiskDev *dev),
	u64 (*read)(hw_DiskDev *dev, u64 off, u64 size, void *buf),
	u64 (*write)(struct hw_DiskDev *dev, u64 off, u64 size, void *buf));

void hw_DiskDev_register(hw_DiskDev *dev);

int hw_DiskDev_install(hw_DiskDev *dev);

int hw_DiskDev_uninstall(hw_DiskDev *dev);

#endif