#include <hal/hardware/uefi.h>
#include <hal/hardware/apic.h>
#include <hal/hardware/cpu.h>
#include <hal/cpu/api.h>
#include <cpu/desc.h>
#include <screen/screen.h>
#include <mm/dmas.h>
#include <lib/string.h>

static u32 _curTrIdx;
static u32 _bspApicId;

static __always_inline__ int _canEnableProc(u32 flag) {
	return (flag & 0x1) || (!(flag & 0x1) && (flag & 0x2));
}

static int _registerCPU(u32 x2apicId, u32 apicId) {
	cpu_Desc *desc = &cpu_desc[cpu_num++];
	memset(desc, 0, sizeof(cpu_Desc));
	printk(WHITE, BLACK, "cpu: x2apic:%#010x apic:%#010x\n", x2apicId, apicId);
	desc->hal.x2apic = x2apicId;
	desc->hal.apic = apicId;
	return res_SUCC;
}

int _parseMadt() {
	hal_hw_uefi_MadtDesc *madt = NULL;
	for (int i = 0; i < (hal_hw_uefi_xsdtTbl->hdr.length - sizeof(hal_hw_uefi_XsdtDesc)) / 8; i++) {
		hal_hw_uefi_AcpiHeader *hdr = mm_dmas_phys2Virt(hal_hw_uefi_xsdtTbl->entry[i]);
		if (strncmp(hdr->signature, "APIC", 4) != 0) continue;
		madt = container(hdr, hal_hw_uefi_MadtDesc, hdr);
		break;
	}
	if (madt == NULL) {
		printk(RED, BLACK, "cpu: cannot for MADT.\n");
		return res_FAIL;
	}
	printk(WHITE, BLACK, "cpu: MADT:%#018lx length=%ld\n", madt, madt->hdr.length);

	// first scan the table for type9 (x2apic)
	for (u64 off = sizeof(hal_hw_uefi_MadtDesc); off < madt->hdr.length; ) {
		hal_hw_uefi_MadtEntry *curEntry = (hal_hw_uefi_MadtEntry *)((u64)madt + off);
		if (curEntry->type == 9 && _canEnableProc(curEntry->type9.flags)) {
			if (_registerCPU(curEntry->type9.x2apicId, curEntry->type9.apicId) == res_FAIL)
				return res_FAIL;
		}
		off += curEntry->len;
	}

	// then scan the table for type0 (apic) use apicId for x2apicId
	for (u64 off = sizeof(hal_hw_uefi_MadtDesc); off < madt->hdr.length; ) {
		hal_hw_uefi_MadtEntry *curEntry = (hal_hw_uefi_MadtEntry *)((u64)madt + off);
		if (curEntry->type == 0 && _canEnableProc(curEntry->type0.flags)) {
			if (_registerCPU(curEntry->type0.apicId, curEntry->type0.apicId) == res_FAIL)
				return res_FAIL;
		}
		off += curEntry->len;
	}
	return res_SUCC;
}

int hal_cpu_init() {
	cpu_num = 0;
	_curTrIdx = 12;
	{
		u32 a, b, c, d;
		if (hal_hw_apic_supportFlag & hal_hw_apic_supportFlag_X2Apic) {
			hal_hw_cpu_cpuid(0xb, 0, &a, &b, &c, &d);
			_bspApicId = d;
			printk(WHITE, BLACK, "cpu: bsp x2apic:%#x\n", _bspApicId);
		} else {
			hal_hw_cpu_cpuid(0x1, 0, &a, &b, &c, &d);
			_bspApicId = b >> 24;
			printk(WHITE, BLACK, "cpu: bsp apic:%#x\n", _bspApicId);
		}
	}
	
	if (_parseMadt() == res_FAIL) return res_FAIL;
	return res_SUCC;
} 