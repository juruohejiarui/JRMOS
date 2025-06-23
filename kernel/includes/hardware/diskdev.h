#ifndef __HARDWARE_DISKDEV_H__
#define __HARDWARE_DISKDEV_H__

#include <lib/dtypes.h>
#include <hardware/mgr.h>

typedef struct hw_DiskDev {
	hw_Device device;
	u64 size;
	// read to buf and return how many bytes has been written to buffer
	u64 (*read)(struct hw_DiskDev *dev, u64 off, u64 size, void *buf);
	// write to device and return hwo many bytes has been written to buffer
	u64 (*write)(struct hw_DiskDev *dev, u64 off, u64 size, void *buf);
} hw_DiskDev;

// this function will not initialize the member "device"
void hw_DiskDev_init(hw_DiskDev *dev, u64 size,
	u64 (*read)(hw_DiskDev *dev, u64 off, u64 size, void *buf),
	u64 (*write)(struct hw_DiskDev *dev, u64 off, u64 size, void *buf));
#endif