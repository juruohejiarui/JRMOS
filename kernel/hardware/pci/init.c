#include <cpu/api.h>
#include <hardware/pci.h>
#include <hal/hardware/pci.h>
#include <screen/screen.h>
#include <mm/mm.h>
#include <lib/bit.h>
#include "devname.h"

SafeList hw_pci_devLst;

intr_Ctrl hw_pci_intrCtrl;

hw_pci_CapHdr *hw_pci_getNxtCap(hw_pci_Cfg *cfg, hw_pci_CapHdr *cur) {
	if (cur) return cur->nxtPtr ? (hw_pci_CapHdr *)((u64)cfg + cur->nxtPtr) : NULL;
	return (hw_pci_CapHdr *)((u64)cfg + cfg->type0.capPtr);
}
static void _chkFunc(u64 baseAddr, u16 bus, u16 dev, u16 func) {
	hw_pci_Cfg *cfg = hw_pci_getCfg(baseAddr, bus, dev, func);
	if (cfg->vendorId == 0xffff) return ;
	if (cfg->class == 0x06 && cfg->subclass == 0x4) return ;

	// printk device information
	hw_pci_Dev *devStruct = mm_kmalloc(sizeof(hw_pci_Dev), mm_Attr_Shared, NULL);
	if (devStruct == NULL) {
		printk(RED, BLACK, "pci: failed to allocate device structure.\n");
		return ;
	}
	devStruct->busId = bus, devStruct->devId = dev, devStruct->funcId = func;
	devStruct->cfg = cfg;
	SafeList_insTail(&hw_pci_devLst, &devStruct->lst);
}

void hw_pci_lstDev() {
	for (ListNode *pciDevNd = SafeList_getHead(&hw_pci_devLst); pciDevNd != &hw_pci_devLst.head; pciDevNd = pciDevNd->next) {
		hw_pci_Dev *dev = container(pciDevNd, hw_pci_Dev, lst);
		printk(WHITE, BLACK, "pci:%02x:%02x:%02x: cls=%04x subcls=%04x progIf=%04x %s\n", 
			dev->busId, dev->devId, dev->funcId, dev->cfg->class, dev->cfg->subclass, dev->cfg->progIf, hw_pci_devName[dev->cfg->class][dev->cfg->subclass]);
	}
}

static void _chkDev(u64 baseAddr, u16 bus, u16 dev) {
	if (hw_pci_getCfg(baseAddr, bus, dev, 0)->vendorId == 0xffff) return ;
	if (hw_pci_getCfg(baseAddr, bus, dev, 0)->hdrType == 0) _chkFunc(baseAddr, bus, dev, 0);
	else for (int i = 0; i < 8; i++) _chkFunc(baseAddr, bus, dev, i);
}

void hw_pci_chkBus(u64 baseAddr, u16 bus) {
	for (u8 dev = 0; dev < 32; dev++) _chkDev(baseAddr, bus, dev);
}

int hw_pci_init() {
    SafeList_init(&hw_pci_devLst);
	if (hal_hw_pci_init() == res_FAIL) return res_FAIL;

	// initialize msi management
	if (hal_hw_pci_initIntr() == res_FAIL) return res_FAIL;

	hw_pci_lstDev();
    return res_SUCC;
}