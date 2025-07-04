#include <hal/hardware/uefi.h>
#include <hal/hardware/apic.h>
#include <hal/hardware/cpu.h>
#include <hal/interrupt/api.h>
#include <hal/init/init.h>
#include <hal/cpu/api.h>
#include <hal/mm/mm.h>
#include <cpu/desc.h>
#include <screen/screen.h>
#include <task/structs.h>
#include <mm/dmas.h>
#include <mm/mm.h>
#include <lib/string.h>
#include <lib/algorithm.h>

static u32 _curTrIdx;
u32 hal_cpu_bspApicId;

__always_inline__ int _canEnableProc(u32 flag) {
	return (flag & 0x1) || (!(flag & 0x1) && (flag & 0x2));
}


static int _registerCPU(u32 x2apicId, u32 apicId) {
	cpu_Desc *desc = &cpu_desc[cpu_num];
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
		memset(desc->hal.tss, 0, sizeof(hal_intr_TSS));
		hal_intr_setTss(desc->hal.tss, 
			(u64)desc->hal.initStk + task_krlStkSize, 0, 0, 
			(u64)desc->hal.initStk + task_krlStkSize, (u64)desc->hal.initStk + task_krlStkSize, 0, 0, 0, 0, 0);
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

	// set cpuId in taskStruct
	task_TaskStruct *tsk = (task_TaskStruct *)desc->hal.initStk;
	tsk->cpuId = cpu_num++;
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
		printk(screen_err, "cpu: cannot for MADT.\n");
		return res_FAIL;
	}
	printk(screen_log, "cpu: MADT:%p length=%ld\n", madt, madt->hdr.length);

	// first scan the table for type9 (x2apic)
	for (u64 off = sizeof(hal_hw_uefi_MadtDesc); off < madt->hdr.length; ) {
		hal_hw_uefi_MadtEntry *curEntry = (hal_hw_uefi_MadtEntry *)((u64)madt + off);
		if (curEntry->tp == 9 && _canEnableProc(curEntry->tp9.flags)) {
			if (_registerCPU(curEntry->tp9.x2apicId, curEntry->tp9.apicId) == res_FAIL)
				return res_FAIL;
		}
		off += curEntry->len;
	}

	// then scan the table for type0 (apic) use apicId for x2apicId
	for (u64 off = sizeof(hal_hw_uefi_MadtDesc); off < madt->hdr.length; ) {
		hal_hw_uefi_MadtEntry *curEntry = (hal_hw_uefi_MadtEntry *)((u64)madt + off);
		if (curEntry->tp == 0 && _canEnableProc(curEntry->tp0.flags)) {
			int exist = 0;
			for (u32 i = 0; i < cpu_num; i++) if (cpu_desc[i].hal.apic == curEntry->tp0.apicId) {
				exist = 1;
				break;
			}
			if (!exist && _registerCPU(curEntry->tp0.apicId, curEntry->tp0.apicId) == res_FAIL)
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
			printk(screen_log, "cpu: bsp x2apic:%#x\n", hal_cpu_bspApicId);
		} else {
			hal_hw_cpu_cpuid(0x1, 0, &a, &b, &c, &d);
			hal_cpu_bspApicId = b >> 24;
			printk(screen_log, "cpu: bsp apic:%#x\n", hal_cpu_bspApicId);
		}
	}
	
	if (_parseMadt() == res_FAIL) return res_FAIL;
	printk(screen_log, "cpu:\n");
	for (int i = 0; i < cpu_num; i++) {
		printk(screen_log, "\t#%d: x2apic:%lx apic:%lx stk:%p idtTbl:%p idtPtr:%p tss:%p\n",
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
		printk(screen_err, "cpu : failed to find BSP in cpu list.\n");
		return res_FAIL;
	}
	printk(screen_log, "cpu: bspIdx:%d\n", cpu_bspIdx);

	// set gsbase
	hal_hw_writeMsr(hal_msr_IA32_GS_BASE, (u64)&cpu_desc[cpu_bspIdx]);
	return res_SUCC;
}

int hal_cpu_enableAP() {
	hal_hw_apic_IcrDesc icr;
	*(u64 *)&icr = 0;
	icr.vector = 0;
	icr.deliverMode = hal_hw_apic_DeliveryMode_INIT;
	icr.destMode = hal_hw_apic_DestMode_Physical;
	icr.deliverStatus = hal_hw_apic_DeliveryStatus_Idle;
	icr.level = hal_hw_apic_Level_Assert;
	icr.triggerMode = hal_hw_apic_TriggerMode_Edge;
	icr.DestShorthand = hal_hw_apic_DestShorthand_AllExcludingSelf;
	
	hal_hw_apic_writeIcr(*(u64 *)&icr);

	// enable interrupt to wait for 
	for (int i = 0; i < 100000000; i++) ;
	
	mm_MemMap *entry;
	u64 stAddr, bootSz = (u64)&hal_cpu_apBootEnd - (u64)&hal_cpu_apBootEntry;
	void *backup = mm_kmalloc(bootSz, mm_Attr_Shared, NULL);
	for (int i = 0; i < mm_nrMemMapEntries; i++) {
		entry = &mm_memMapEntries[i];
		stAddr = upAlign(max(0x2000, entry->addr), Page_4KSize);
		if (entry->attr & mm_Attr_Firmware 
		 || stAddr >= 0x100000
		 || bootSz > upAlign(entry->addr + entry->size, Page_4KSize) - stAddr) { 
			entry = NULL; continue;
		}
		break;
	}
	if (entry == NULL) {
		printk(screen_err, "cpu: failed to find valid space for apu boot.\n");
		return res_FAIL;
	}
	icr.vector = stAddr >> Page_4KShift;
	icr.deliverMode = hal_hw_apic_DeliveryMode_Startup;
	icr.DestShorthand = hal_hw_apic_DestShorthand_None;

	printk(screen_log, "cpu: startup vector: %#x sz=%ld backup=%p\n", icr.vector, bootSz, backup);
	memcpy(mm_dmas_phys2Virt(icr.vector << Page_4KShift), backup, bootSz);
	memcpy(&hal_cpu_apBootEntry, mm_dmas_phys2Virt(icr.vector << Page_4KShift), bootSz);

	for (int i = 0; i < cpu_num; i++) if (i != cpu_bspIdx) {
		printk(screen_log, "cpu: try enabling proc#%d...\r", i);
		cpu_Desc *cpu = &cpu_desc[i];
		hal_hw_apic_setIcrDest(&icr, cpu->hal.apic, cpu->hal.x2apic);
		hal_hw_apic_writeIcr(*(u64 *)&icr);

		hal_hw_apic_writeIcr(*(u64 *)&icr);

		while (cpu->state != cpu_Desc_state_Active) ;
	}

	// copy it back
	memcpy(backup, mm_dmas_phys2Virt(icr.vector << Page_4KShift), bootSz);
	mm_kfree(backup, mm_Attr_Shared);
	
	// cancel the map 0x0(virt) -> 0x0(phys)
	for (int i = 0; i < 256; i++) ((u64 *)mm_dmas_phys2Virt(mm_krlTblPhysAddr))[i] = 0;
	return res_SUCC;
}

void hal_cpu_chk() {
	u32 a, b, c, d;
	hal_hw_cpu_cpuid(0x1, 0x0, &a, &b, &c, &d);
	u64 flags = 0;
	if (c & (1u << 20)) flags |= hal_cpu_flags_sse42;
	if (c & (1u << 26)) flags |= hal_cpu_flags_xsave;
	if (c & (1u << 27)) flags |= hal_cpu_flags_osxsave;
	if (c & (1u << 27)) flags |= hal_cpu_flags_avx;

	hal_cpu_setvar(hal.flags, flags);

	printk(screen_log, "cpu #%d: sse42[%c] xsave[%c] osxsave[%c] avx[%c]\n", 
		hal_cpu_getvar(hal.apic), 
		(flags & hal_cpu_flags_sse42   ? 'y' : 'n'),
		(flags & hal_cpu_flags_xsave   ? 'y' : 'n'),
		(flags & hal_cpu_flags_osxsave ? 'y' : 'n'),
		(flags & hal_cpu_flags_avx     ? 'y' : 'n')
	);
}