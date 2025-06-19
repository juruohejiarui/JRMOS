#include <hardware/nvme.h>
#include <screen/screen.h>
#include <mm/mm.h>

hw_Driver hw_nvme_drv;

int hw_nvme_chk(hw_Device *dev) {
	if (dev->parent != &hw_pci_rootDev) return res_FAIL;
	hw_pci_Dev *pciDev = container(dev, hw_pci_Dev, device);
	if (pciDev->cfg->class != 0x01 || pciDev->cfg->subclass != 0x08) return res_FAIL;
	printk(WHITE, BLACK, "hw: nvme: find controller: %02x:%02x:%01x vendor=%04x device=%04x\n",
		pciDev->busId, pciDev->devId, pciDev->funcId, pciDev->cfg->vendorId, pciDev->cfg->deviceId);
	return res_SUCC;
}

__always_inline__ int _getRegAddr(hw_nvme_Host *host) {
	host->capRegAddr = (u64)mm_dmas_phys2Virt(hw_pci_Cfg_getBar(&host->pci.cfg->type0.bar[0]));

	// map regsters
	return mm_dmas_map(mm_dmas_virt2Phys(host->capRegAddr));
}

__always_inline__ int _initHost(hw_nvme_Host *host) {
	if (_getRegAddr(host) == res_FAIL) return res_FAIL;

	printk(WHITE, BLACK, "hw: nvme: %p: capAddr:%p cap:%#018lx version:%x\n", 
		host, host->capRegAddr, hw_nvme_read64(host, hw_nvme_Host_ctrlCap), hw_nvme_read32(host, hw_nvme_Host_version));

	
	
	return res_SUCC;
}

int hw_nvme_cfg(hw_Device *dev) {
	hw_pci_Dev *pciDev = container(dev, hw_pci_Dev, device);

	hw_nvme_Host *host = mm_kmalloc(sizeof(hw_nvme_Host), mm_Attr_Shared, NULL);
	
	hw_pci_extend(pciDev, &host->pci);

	return _initHost(host);
}

void hw_nvme_init() {
	hw_driver_initDriver(&hw_nvme_drv, "hw: nvme", hw_nvme_chk, hw_nvme_cfg, NULL, NULL);

	hw_driver_register(&hw_nvme_drv);
}