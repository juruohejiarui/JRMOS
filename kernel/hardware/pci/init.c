#include <cpu/api.h>
#include <hardware/pci.h>
#include <hal/hardware/pci.h>
#include <screen/screen.h>
#include <mm/mm.h>
#include <lib/bit.h>
#include "devname.h"

SafeList hw_pci_devLst;

hw_Device hw_pci_rootDev;

intr_Ctrl hw_pci_intrCtrl;

void hw_pci_extend(hw_pci_Dev *origin, hw_pci_Dev *ext) {
	ListNode *prev = origin->lst.prev;
	SafeList_del(&hw_pci_devLst, &origin->lst);
	memcpy(origin, ext, sizeof(hw_pci_Dev));
	mm_kfree(origin, mm_Attr_Shared);
	SafeList_insBehind(&hw_pci_devLst, &ext->lst, prev);
}

hw_pci_CapHdr *hw_pci_getNxtCap(hw_pci_Cfg *cfg, hw_pci_CapHdr *cur) {
	if (cur) return cur->nxtPtr ? (hw_pci_CapHdr *)((u64)cfg + cur->nxtPtr) : NULL;
	return (hw_pci_CapHdr *)((u64)cfg + cfg->tp0.capPtr);
}
static void _chkFunc(u64 baseAddr, u16 bus, u16 dev, u16 func) {
	hw_pci_Cfg *cfg = hw_pci_getCfg(baseAddr, bus, dev, func);
	if (cfg->vendorId == 0xffff) return ;
	if (cfg->class == 0x06 && cfg->subclass == 0x4) return ;

	// printk device information
	hw_pci_Dev *devStruct = mm_kmalloc(sizeof(hw_pci_Dev), mm_Attr_Shared, NULL);
	if (devStruct == NULL) {
		printk(screen_err, "pci: failed to allocate device structure.\n");
		return ;
	}
	devStruct->busId = bus, devStruct->devId = dev, devStruct->funcId = func;
	devStruct->cfg = cfg;
	devStruct->device.parent = &hw_pci_rootDev;
	devStruct->device.drv = NULL;
	SafeList_insTail(&hw_pci_devLst, &devStruct->lst);
}

void hw_pci_lstDev() {
	SafeList_enum(&hw_pci_devLst, pciDevNd) {
		hw_pci_Dev *dev = container(pciDevNd, hw_pci_Dev, lst);
		printk(screen_log, "pci:%02x:%02x:%01x: device=%04x vendor=%04x hdrTp=%02x cls=%04x subcls=%04x progIf=%04x %s \n", 
			dev->busId, dev->devId, dev->funcId, 
			dev->cfg->deviceId, dev->cfg->vendorId, dev->cfg->hdrTp, 
			dev->cfg->class, dev->cfg->subclass, dev->cfg->progIf, 
			({const void *ptr = hw_pci_devName[dev->cfg->class][dev->cfg->subclass]; ptr != NULL ? ptr : ""; })
		);
	}
}

int hw_pci_chk() {
	printk(screen_log, "hw: pci: checking\n");
	SafeList_enum(&hw_pci_devLst, pciDevNd) {
		hw_pci_Dev *dev = container(pciDevNd, hw_pci_Dev, lst);
		// scan drivers
		hw_Driver *drv;
		SafeList_enum(&hw_drvLst, drvNd) {
			hw_Driver *drv = container(drvNd, hw_Driver, lst);
			// find one driver
			if (drv->check(&dev->device) == res_SUCC) {
				dev->device.drv = drv;
				SafeList_exitEnum(&hw_drvLst);
			}
		}
	}
	int res = res_SUCC;
	for (ListNode *cur = hw_pci_devLst.head.next, *nxt = cur->next; 
			cur != &hw_pci_devLst.head;
			cur = nxt, nxt = cur->next) {
		hw_pci_Dev *dev = container(cur, hw_pci_Dev, lst);
		if (dev->device.drv != NULL)
			if (dev->device.drv->cfg != NULL) res |= dev->device.drv->cfg(&dev->device);
	}
	SafeList_enum(&hw_pci_devLst, pciDevNd) {
		hw_pci_Dev *dev = container(pciDevNd, hw_pci_Dev, lst);
		if (dev->device.drv != NULL && dev->device.drv->install != NULL)
			res |= dev->device.drv->install(&dev->device);
	}
	return res;
}

static void _chkDev(u64 baseAddr, u16 bus, u16 dev) {
	if (hw_pci_getCfg(baseAddr, bus, dev, 0)->vendorId == 0xffff) return ;
	if (hw_pci_getCfg(baseAddr, bus, dev, 0)->hdrTp == 0) _chkFunc(baseAddr, bus, dev, 0);
	else for (int i = 0; i < 8; i++) _chkFunc(baseAddr, bus, dev, i);
}

void hw_pci_chkBus(u64 baseAddr, u16 bus) {
	for (u8 dev = 0; dev < 32; dev++) _chkDev(baseAddr, bus, dev);
}

int hw_pci_init() {
	hw_registerRootDev(&hw_pci_rootDev);
    SafeList_init(&hw_pci_devLst);
	if (hal_hw_pci_init() == res_FAIL) return res_FAIL;

	// initialize msi management
	if (hal_hw_pci_initIntr() == res_FAIL) return res_FAIL;

	return res_SUCC;
}