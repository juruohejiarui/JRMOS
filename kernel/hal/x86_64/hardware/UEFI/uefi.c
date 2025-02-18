#include <hal/hardware/uefi.h>
#include <task/constant.h>
#include <screen/screen.h>

hal_hw_uefi_InfoStruct *hal_hw_uefi_info;

static hal_hw_uefi_RsdpDesc *_rsdpTbl;
static hal_hw_uefi_XsdtDesc *_xsdtTbl;

void hal_hw_uefi_init() {
	hal_hw_uefi_info = (void *)(0x60000ul + Task_krlAddrSt);
	screen_info = &hal_hw_uefi_info->screenInfo;
	_rsdpTbl = NULL;
	_xsdtTbl = NULL;
}

int hal_hw_uefi_loadTbl() {

}