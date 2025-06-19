#ifndef __HADEWARE_NVME_H__
#define __HARDWARE_NVME_H__

#include <hardware/pci.h>
#include <hardware/mgr.h>

typedef struct hw_nvme_Driver {
	hw_Driver driver;
	
} hw_nvme_Driver;

void hw_nvme_init();
#endif