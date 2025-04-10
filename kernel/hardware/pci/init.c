#include <cpu/api.h>
#include <hardware/pci.h>
#include <hal/hardware/pci.h>
#include <screen/screen.h>
#include "devname.h"
#include "pci.h"

SafeList hw_pci_devLst;

intr_Ctrl hw_pci_intrCtrl;

static void _chkFunc(u64 baseAddr, u16 bus, u16 dev, u16 func) {
	hw_pci_Cfg *cfg = hw_pci_getCfg(baseAddr, bus, dev, func);
	if (cfg->vendorId == 0xffff) return ;
	if (cfg->class == 0x06 && cfg->subclass == 0x4) return ;

	// printk device information
	printk(WHITE, BLACK, "pci:%02x:%02x:%02x: cls=%04x subcls=%04x progIf=%04x %s\n", bus, dev, func, cfg->class, cfg->subclass, cfg->progIf, hw_pci_devName[cfg->class][cfg->subclass]);
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
	if (hal_hw_pci_enum() == res_FAIL) return res_FAIL;

	// initialize msi management
	if (hal_hw_pci_initIntr() == res_FAIL) return res_FAIL;
    return res_SUCC;
}

void hw_pci_initIntr(intr_Desc *desc, void (*handler)(u64), u64 param, char *name) {
	desc->handler = handler;
	desc->param = param;
	desc->name = name;
	desc->ctrl = &hw_pci_intrCtrl;
}

int hw_pci_allocMsi(hw_pci_MsiCap *cap, intr_Desc *desc, u64 intrNum) {
	if (hw_pci_MsiCap_vecNum(cap) < intrNum) {
		printk(RED, BLACK, "hw: pci: alloc too much interrupt for msi, vecNum=%d, require %d interrupt\n", hw_pci_MsiCap_vecNum(cap), intrNum);
		return res_FAIL;
	}
	if (intr_alloc(desc, intrNum) == res_FAIL) {
		printk(RED, BLACK, "hw: pci: alloc msi interrupt failed\n");
		return res_FAIL;
	}
	if (hal_hw_setMsiIntr(cap, desc, intrNum) == res_FAIL) {
		printk(RED, BLACK, "hw: pci: set msi interrupt failed\n");
		intr_free(desc, intrNum);
		return res_FAIL;
	}
	return res_FAIL;
}