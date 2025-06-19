#include <hal/hardware/gpu/intel.h>
#include <hardware/pci.h>
#include <screen/screen.h>

static int _analyseGeneration(u32 deviceId) {
	switch (deviceId & 0xfff0) {
		case 0xa780 :
			printk(WHITE, BLACK, "hw: gpu: intel: Raptor Lake\n");
			break;
		default :
			printk(RED, BLACK, "hw: gpu: intel: unsupported GPU.\n");
			return res_FAIL;
	}
	return res_SUCC;
}

int hal_hw_gpu_intel_init() {
	hw_pci_Dev *dev;
	SafeList_enum(&hw_pci_devLst, devNd) {
		dev = container(devNd, hw_pci_Dev, lst);
		if (dev->cfg->vendorId == 0x8086 && dev->cfg->class == 0x03 && dev->cfg->subclass == 0x80) {
			SafeList_exitEnum(&hw_pci_devLst);
		} else dev = NULL;
	}
	if (dev == NULL) {
		printk(YELLOW, BLACK, "hw: gpu: intel GPU not found.\n");
		return res_FAIL;
	}
	if (_analyseGeneration(dev->cfg->deviceId) != res_SUCC) return res_FAIL;
	printk(GREEN, BLACK, "hw: gpu: find intel GPU: %02x:%02x:%02x %04x:%04x\n",
		dev->busId, dev->devId, dev->funcId, dev->cfg->vendorId, dev->cfg->deviceId);
	return res_SUCC;
}