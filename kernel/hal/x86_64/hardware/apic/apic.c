#include <hal/hardware/apic.h>
#include <hal/hardware/cpu.h>
#include <screen/screen.h>
#include <mm/mm.h>
#include <mm/map.h>
#include <mm/buddy.h>
#include <lib/bit.h>
#include <hal/hardware/io.h>
#include <lib/string.h>

// the index of RTE register of the specific vector
#define _idxOfRte(vecId) (((vecId - 0x20) << 1) + 0x10)
struct ioApicMap {
	u32 phyAddr;
	u8 *idx;
	u32 *data;
	u32 *eoi;
} ioApicMap;

static mm_Page *_locApicPg;

u64 hal_hw_apic_supportFlag;

intr_Ctrl hal_hw_apic_intrCtrl;

int hal_hw_apic_map() {
	if (mm_dmas_map(0xfec00000) == res_FAIL) return res_FAIL;
	if (mm_dmas_map(0xfee00000) == res_FAIL) return res_FAIL;
	ioApicMap.phyAddr = 0xfec00000;
	ioApicMap.idx = mm_dmas_phys2Virt(ioApicMap.phyAddr);
	ioApicMap.data = mm_dmas_phys2Virt(ioApicMap.phyAddr + 0x10);
	ioApicMap.eoi = mm_dmas_phys2Virt(ioApicMap.phyAddr + 0x40);
	return res_SUCC;
}

int hal_hw_apic_initLocal() {
	hal_hw_apic_supportFlag = 0;
	printk(RED, BLACK, "hw: apic: init local.\n");

	_locApicPg = mm_allocPages(0, mm_Attr_Shared);
	memset(mm_dmas_phys2Virt(mm_getPhyAddr(_locApicPg)), 0, mm_pageSize);
	hal_hw_apic_setApicBase(mm_getPhyAddr(_locApicPg));

	u32 a, b, c, d;
	hal_hw_cpu_cpuid(1, 0, &a, &b, &c, &d);
	printk(WHITE, BLACK, "CPUID\t01: a:%#010x,b:%#010x,c:%#010x,d:%#010x\n", a, b, c, d);

	printk(WHITE, BLACK, "hw: apic: support: ");
	if (d & (1ul << 9))
		printk(GREEN, BLACK, "xAPIC ");
	else {
		printk(RED, BLACK, "xAPIC ");
		return res_FAIL;
	}
	
	if (c & (1ul << 21))
		printk(GREEN, BLACK, "x2APIC "),
		hal_hw_apic_supportFlag |= hal_hw_apic_supportFlag_X2Apic;
	else printk(RED, BLACK, "x2APIC ");

	if (*(u64 *)mm_dmas_phys2Virt(0xfee00030) & (1ul << 24))
		printk(GREEN, BLACK, "EOI-broadcase\n"),
		hal_hw_apic_supportFlag |= hal_hw_apic_supportFlag_EOIBroadcase;
	else printk(RED, BLACK, "EOI-broadcase\n");

	{
		u64 val = hal_hw_readMsr(0x1b);
		bit_set1(&val, 11);
		if (hal_hw_apic_supportFlag & hal_hw_apic_supportFlag_X2Apic) bit_set1(&val, 10);
		hal_hw_writeMsr(0x1b, val);
	}

	printk(WHITE, BLACK, "hw: apic: support: ");
	u32 x, y;
	x = hal_hw_readMsr(0x1b);
	printk(((x & 0x800) ? GREEN : RED), BLACK, "xAPIC ");
	printk(((x & 0x400) ? GREEN : RED), BLACK, "x2APIC\n");

	// enable SVR[8] and SVR[12]
	{
		u64 val = (hal_hw_apic_supportFlag & hal_hw_apic_supportFlag_X2Apic ? hal_hw_readMsr(0x80f) : *(u64 *)mm_dmas_phys2Virt(0xfee000f0));
		bit_set1(&val, 8);
		if (hal_hw_apic_supportFlag & hal_hw_apic_supportFlag_EOIBroadcase) bit_set1(&val, 12);
		if (hal_hw_apic_supportFlag & hal_hw_apic_supportFlag_X2Apic) hal_hw_writeMsr(0x80f, val);
		else {
			*(u64 *)mm_dmas_phys2Virt(0xfee000f0) = val;
			*(u64 *)mm_dmas_phys2Virt(0xfee002f0) = 0x100000;
		}
	}
	return res_SUCC;
}

int hal_hw_apic_initIO() {
	return res_SUCC;
}

int hal_hw_apic_init() {
	if (hal_hw_apic_map() == res_FAIL) {
		printk(RED, BLACK, "hw: apic: failed to map.\n");
		return res_FAIL;
	}
	hal_hw_io_out8(0x21, 0xff);
	hal_hw_io_out8(0xa1, 0xff);
	
	hal_hw_io_out8(0x22, 0x70);
	hal_hw_io_out8(0x23, 0x01);


	if (hal_hw_apic_initLocal() == res_FAIL) return res_FAIL;

	hal_hw_io_out32(0xcf8, 0x8000f8f0);
	hal_hw_mfence();

	hal_hw_apic_intrCtrl.ack = hal_hw_apic_edgeAck;
	hal_hw_apic_intrCtrl.install = hal_hw_apic_install;
	hal_hw_apic_intrCtrl.uninstall = hal_hw_apic_uninstall;
	hal_hw_apic_intrCtrl.enable = hal_hw_apic_enable;
	hal_hw_apic_intrCtrl.disable = hal_hw_apic_disable;

	return res_SUCC;
}

void hal_hw_apic_writeRte(u8 idx, u64 val) {
	*ioApicMap.idx = idx;
	hal_hw_mfence();
	*ioApicMap.data = val & 0xfffffffful;
	hal_hw_mfence();

	*ioApicMap.idx = idx + 1;
	hal_hw_mfence();
	*ioApicMap.data = val >> 32;
	hal_hw_mfence();
}

u64 hal_hw_apic_readRte(u8 idx) {
	u64 val;
	*ioApicMap.idx = idx;
	hal_hw_mfence();
	val = *ioApicMap.data;
	hal_hw_mfence();

	*ioApicMap.idx = idx + 1;
	hal_hw_mfence();
	val |= ((u64)*ioApicMap.data) << 32;
	hal_hw_mfence();

	return val;
}

void hal_hw_apic_writeIcr(u64 val) {
	if (hal_hw_apic_supportFlag & hal_hw_apic_supportFlag_X2Apic) hal_hw_writeMsr(0x830, val);
	else {
		*(u32 *)mm_dmas_phys2Virt(0xfee00310) = (u32)(val >> 32);
		hal_hw_mfence();
		*(u32 *)mm_dmas_phys2Virt(0xfee00300) = val & 0xffffffffu;
	}
}

int hal_hw_apic_install(intr_Desc *desc, void *arg) {
	u64 val = *(u64 *)arg;
	((hal_hw_apic_RteDesc *)&val)->mask = hal_hw_apic_Mask_Masked;
	hal_hw_apic_writeRte(_idxOfRte(desc->vecId), val);
	return res_SUCC;
}

int hal_hw_apic_uninstall(intr_Desc *desc) {
	hal_hw_apic_writeRte(_idxOfRte(desc->vecId), (1ul << 16));
	return res_SUCC;
}

int hal_hw_apic_enable(intr_Desc *desc) {
	u64 rte = hal_hw_apic_readRte(_idxOfRte(desc->vecId));
	((hal_hw_apic_RteDesc *)&rte)->mask = hal_hw_apic_Mask_Unmasked;
	hal_hw_apic_writeRte(_idxOfRte(desc->vecId), rte);
	return res_SUCC;
}

int hal_hw_apic_disable(intr_Desc *desc) {
	u64 rte = hal_hw_apic_readRte(_idxOfRte(desc->vecId));
	((hal_hw_apic_RteDesc *)&rte)->mask = hal_hw_apic_Mask_Masked;
	hal_hw_apic_writeRte(_idxOfRte(desc->vecId), rte);
	return res_SUCC;
}

void hal_hw_apic_edgeAck(intr_Desc *desc) {
	if (hal_hw_apic_supportFlag & hal_hw_apic_supportFlag_X2Apic) {
		__asm__ volatile (
			"movq $0x00, %%rdx		\n\t"
			"movq $0x00, %%rax		\n\t"
			"movq $0x80b, %%rcx		\n\t"
			"wrmsr					\n\t"
			:
			:
			: "memory", "rdx", "rax", "rcx"
		);
	} else *(u32 *)mm_dmas_phys2Virt(0xfee000b0) = 0;
}