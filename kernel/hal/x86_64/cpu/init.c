#include <hal/hardware/uefi.h>
#include <hal/hardware/apic.h>
#include <hal/hardware/cpu.h>
#include <hal/interrupt/api.h>
#include <hal/init/init.h>
#include <hal/cpu/api.h>
#include <cpu/desc.h>
#include <screen/screen.h>
#include <mm/dmas.h>
#include <mm/mm.h>
#include <lib/string.h>

static u32 _curTrIdx;
u32 hal_cpu_bspApicId;

static __always_inline__ int _canEnableProc(u32 flag) {
	return (flag & 0x1) || (!(flag & 0x1) && (flag & 0x2));
}


static int _registerCPU(u32 x2apicId, u32 apicId) {
	cpu_Desc *desc = &cpu_desc[cpu_num++];
	memset(desc, 0, sizeof(cpu_Desc));
	desc->hal.x2apic = x2apicId;
	desc->hal.apic = apicId;
	if (x2apicId != hal_cpu_bspApicId) {
		desc->hal.initStk = mm_kmalloc(task_krlStkSize, mm_Attr_Shared, NULL);

		desc->hal.idtTbl = mm_kmalloc(sizeof(hal_intr_IdtItem) * 0x100, mm_Attr_Shared, NULL);
		desc->hal.idtTblSz = sizeof(hal_intr_IdtItem) * 0x100;
		memcpy(hal_init_idtTbl, desc->hal.idtTbl, sizeof(hal_intr_IdtItem) * 0x100);

		desc->hal.trIdx = _curTrIdx;
		_curTrIdx += 2;
		desc->hal.tss = mm_kmalloc(128, mm_Attr_Shared, NULL);
		hal_intr_setTssItem(desc->hal.trIdx, desc->hal.tss);
	} else {
		desc->hal.initStk = hal_init_stk;
		desc->hal.idtTbl = hal_init_idtTbl;
		desc->hal.idtTblSz = sizeof(hal_intr_IdtItem) * 0x100;
		desc->hal.tss = (hal_intr_TSS *)hal_init_tss;
		desc->hal.trIdx = 10;
	}
	desc->intrMsk[0] = desc->intrMsk[2] = desc->intrMsk[3] = ~0x0ul;
	desc->intrFree = 64;
	desc->intrUsage = 64 * 3;
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
			int exist = 0;
			for (u32 i = 0; i < cpu_num; i++) if (cpu_desc[i].hal.apic == curEntry->type0.apicId) {
				exist = 1;
				break;
			}
			if (!exist && _registerCPU(curEntry->type0.apicId, curEntry->type0.apicId) == res_FAIL)
				return res_FAIL;
		}
		off += curEntry->len;
	}
	return res_SUCC;
}

int hal_cpu_init() {
	if (hal_cpu_initIntr() == res_FAIL) return res_FAIL;

	cpu_num = 0;
	_curTrIdx = 12;
	{
		u32 a, b, c, d;
		if (hal_hw_apic_supportFlag & hal_hw_apic_supportFlag_X2Apic) {
			hal_hw_cpu_cpuid(0xb, 0, &a, &b, &c, &d);
			hal_cpu_bspApicId = d;
			printk(WHITE, BLACK, "cpu: bsp x2apic:%#x\n", hal_cpu_bspApicId);
		} else {
			hal_hw_cpu_cpuid(0x1, 0, &a, &b, &c, &d);
			hal_cpu_bspApicId = b >> 24;
			printk(WHITE, BLACK, "cpu: bsp apic:%#x\n", hal_cpu_bspApicId);
		}
	}
	
	if (_parseMadt() == res_FAIL) return res_FAIL;
	printk(WHITE, BLACK, "cpu:\n");
	for (int i = 0; i < cpu_num; i++) {
		printk(WHITE, BLACK, "\t#%d: x2apic:%lx apic:%lx stk:%#018lx idtTbl:%#018lx idtPtr:%#018lx tss:%#018lx\n",
			i, cpu_desc[i].hal.x2apic, cpu_desc[i].hal.apic, cpu_desc[i].hal.initStk, cpu_desc[i].hal.idtTbl, &cpu_desc[i].hal.idtTblSz,
			cpu_desc[i].hal.tss);
	}

	cpu_bspIdx = cpu_mxNum;
	for (int i = 0; i < cpu_num; i++)
		if (hal_cpu_bspApicId == cpu_desc[i].hal.x2apic) {
			cpu_bspIdx = i;
			break;
		}
	if (cpu_bspIdx == cpu_mxNum) {
		printk(RED, BLACK, "cpu : failed to find BSP in cpu list.\n");
		return res_FAIL;
	}
	printk(WHITE, BLACK, "cpu: bspIdx:%d\n", cpu_bspIdx);
	return res_SUCC;
}

int hal_cpu_enableAP() {
	
}