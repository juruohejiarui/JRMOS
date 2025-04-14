#include <hardware/pci.h>
#include <cpu/api.h>
#include <task/api.h>
#include <hal/hardware/pci.h>
#include <hal/hardware/uefi.h>
#include <hal/hardware/apic.h>
#include <mm/dmas.h>
#include <lib/string.h>
#include <screen/screen.h>

static hal_hw_uefi_McfgDesc *_mfcgDesc;


int hal_hw_pci_enum() {
	_mfcgDesc = NULL;
	u32 entryCnt = (hal_hw_uefi_xsdtTbl->hdr.length - sizeof(hal_hw_uefi_xsdtTbl)) / sizeof(u64);
	for (int i = 0; i < entryCnt; i++) {
		hal_hw_uefi_AcpiHeader *hdr = (hal_hw_uefi_AcpiHeader *)mm_dmas_phys2Virt(hal_hw_uefi_xsdtTbl->entry[i]);
		if (!strncmp(hdr->signature, "MCFG", 4)) {
			_mfcgDesc = (hal_hw_uefi_McfgDesc *)hdr;
			break;
		}
	}
	if (_mfcgDesc == NULL) {
		printk(RED, BLACK, "hw: pci: MCFG not found\n");
		return res_FAIL;
	}
	printk(WHITE, BLACK, "hw: pci: MCFG: %#018lx\n", _mfcgDesc);
	u32 _devCnt = (_mfcgDesc->hdr.length - sizeof(hal_hw_uefi_McfgDesc)) / sizeof(struct hal_hw_uefi_McfgDescEntry);
	for (int i = 0; i < _devCnt; i++) {
		for (u16 bus = _mfcgDesc->entry[i].stBus; bus <= _mfcgDesc->entry[i].edBus; bus++) hw_pci_chkBus(_mfcgDesc->entry[i].addr, bus);
	}
	return res_SUCC;
}

int hal_hw_pci_initIntr() {
	memset(&hw_pci_intrCtrl, 0, sizeof(intr_Ctrl));
	hw_pci_intrCtrl.ack = hal_hw_apic_edgeAck;
	return res_SUCC;
}

