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

#define buildIrq(num) hal_intr_buildIrq(pci_intr, num, hal_hw_pci_intr_dispatcher);

// 0x40 ~ 0x7f
buildIrq(0x40)	buildIrq(0x41)	buildIrq(0x42)	buildIrq(0x43)	buildIrq(0x44)	buildIrq(0x45)	buildIrq(0x46)	buildIrq(0x47)
buildIrq(0x48)	buildIrq(0x49)	buildIrq(0x4a)	buildIrq(0x4b)	buildIrq(0x4c)	buildIrq(0x4d)	buildIrq(0x4e)	buildIrq(0x4f)
buildIrq(0x50)	buildIrq(0x51)	buildIrq(0x52)	buildIrq(0x53)	buildIrq(0x54)	buildIrq(0x55)	buildIrq(0x56)	buildIrq(0x57)
buildIrq(0x58)	buildIrq(0x59)	buildIrq(0x5a)	buildIrq(0x5b)	buildIrq(0x5c)	buildIrq(0x5d)	buildIrq(0x5e)	buildIrq(0x5f)
buildIrq(0x60)	buildIrq(0x61)	buildIrq(0x62)	buildIrq(0x63)	buildIrq(0x64)	buildIrq(0x65)	buildIrq(0x66)	buildIrq(0x67)
buildIrq(0x68)	buildIrq(0x69)	buildIrq(0x6a)	buildIrq(0x6b)	buildIrq(0x6c)	buildIrq(0x6d)	buildIrq(0x6e)	buildIrq(0x6f)
buildIrq(0x70)	buildIrq(0x71)	buildIrq(0x72)	buildIrq(0x73)	buildIrq(0x74)	buildIrq(0x75)	buildIrq(0x76)	buildIrq(0x77)
buildIrq(0x78)	buildIrq(0x79)	buildIrq(0x7a)	buildIrq(0x7b)	buildIrq(0x7c)	buildIrq(0x7d)	buildIrq(0x7e)	buildIrq(0x7f)

void (*hal_hw_pci_intrLst[0x40])(void) = {
	pci_intr0x40, pci_intr0x41, pci_intr0x42, pci_intr0x43,
	pci_intr0x44, pci_intr0x45, pci_intr0x46, pci_intr0x47,
	pci_intr0x48, pci_intr0x49, pci_intr0x4a, pci_intr0x4b,
	pci_intr0x4c, pci_intr0x4d, pci_intr0x4e, pci_intr0x4f,
	pci_intr0x50, pci_intr0x51, pci_intr0x52, pci_intr0x53,
	pci_intr0x54, pci_intr0x55, pci_intr0x56, pci_intr0x57,
	pci_intr0x58, pci_intr0x59, pci_intr0x5a, pci_intr0x5b,
	pci_intr0x5c, pci_intr0x5d, pci_intr0x5e, pci_intr0x5f,
	pci_intr0x60, pci_intr0x61, pci_intr0x62, pci_intr0x63,
	pci_intr0x64, pci_intr0x65, pci_intr0x66, pci_intr0x67,
	pci_intr0x68, pci_intr0x69, pci_intr0x6a, pci_intr0x6b,
	pci_intr0x6c, pci_intr0x6d, pci_intr0x6e, pci_intr0x6f,
	pci_intr0x70, pci_intr0x71, pci_intr0x72, pci_intr0x73,
	pci_intr0x74, pci_intr0x75, pci_intr0x76, pci_intr0x77,
	pci_intr0x78, pci_intr0x79, pci_intr0x7a, pci_intr0x7b,
	pci_intr0x7c, pci_intr0x7d, pci_intr0x7e, pci_intr0x7f
};


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

void hal_hw_pci_intr_dispatcher(u64 rsp, u64 num) {
	intr_Desc *intr = cpu_getvar(hal.intrDesc[num]);
	if (intr == NULL || intr->handler == NULL) {
		printk(RED, BLACK, "hw: pci: no handler for intr #%d on cpu #%d\n", num, task_current->cpuId);
		while (1) hal_hw_hlt();
	}
	intr->handler(intr->param);
	if (intr->ctrl != NULL && intr->ctrl->ack != NULL) intr->ctrl->ack(intr);
}