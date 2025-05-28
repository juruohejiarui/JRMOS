#include <hal/hardware/uefi.h>
#include <task/constant.h>
#include <screen/screen.h>
#include <mm/dmas.h>
#include <lib/string.h>

hal_hw_uefi_InfoStruct *hal_hw_uefi_info;

hal_hw_uefi_RsdpDesc *hal_hw_uefi_rsdpTbl;
hal_hw_uefi_XsdtDesc *hal_hw_uefi_xsdtTbl;

void hal_hw_uefi_init() {
	hal_hw_uefi_info = (void *)(0x60000ul + task_krlAddrSt);
	screen_info = &hal_hw_uefi_info->screenInfo;
	hal_hw_uefi_rsdpTbl = NULL;
	hal_hw_uefi_xsdtTbl = NULL;
}

int hal_hw_uefi_loadTbl() {
	u8 tmpData4[8]= hal_hw_uefi_GUID_ACPI2_data4;
	i64 rsdpIdx = -1;
	for (u64 i = 0; i < hal_hw_uefi_info->cfgTblCnt; i++) {
		hal_hw_uefi_GUID *guid = &((hal_hw_uefi_CfgTbl *)mm_dmas_phys2Virt(hal_hw_uefi_info->cfgTbls))[i].venderGUID;
		if (guid->data1 != hal_hw_uefi_GUID_ACPI2_data1 
			|| guid->data2 != hal_hw_uefi_GUID_ACPI2_data2
			|| guid->data3 != hal_hw_uefi_GUID_ACPI2_data3
			|| memcmp(guid->data4, tmpData4, sizeof(tmpData4)))
			continue;
		rsdpIdx = i;
		break;
	}
	if (rsdpIdx == -1) {
		printk(RED, BLACK, "hw: uefi: cannot find ACPI Table.\n");
		return res_FAIL;
	}
	hal_hw_uefi_rsdpTbl = (hal_hw_uefi_RsdpDesc *)mm_dmas_phys2Virt(
		((hal_hw_uefi_CfgTbl *)mm_dmas_phys2Virt(hal_hw_uefi_info->cfgTbls))[rsdpIdx].vendorTbl);
	hal_hw_uefi_xsdtTbl = (hal_hw_uefi_XsdtDesc *)mm_dmas_phys2Virt(hal_hw_uefi_rsdpTbl->xsdtAddr);

	printk(WHITE, BLACK, "hw: uefi: rsdp:%p xsdt:%p\n", hal_hw_uefi_rsdpTbl, hal_hw_uefi_xsdtTbl);
	return res_SUCC;
}