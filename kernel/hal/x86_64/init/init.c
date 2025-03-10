#include <hal/init/init.h>
#include <hal/hardware/uefi.h>
#include <hal/hardware/reg.h>
#include <init/init.h>
#include <interrupt/api.h>
#include <hal/cpu/api.h>
#include <mm/mm.h>
#include <mm/map.h>
#include <mm/buddy.h>
#include <screen/screen.h>
#include <hal/timer/api.h>

u8 hal_init_stk[task_krlStkSize] __attribute__((__section__ (".data.hal_init_stk") )) = {};

// this is the function called by head.S
void hal_init_init() {
	// get the information from uefi table
	hal_hw_uefi_init();

	screen_init();
	
	int res = mm_init();
	if (res == res_FAIL) while (1) ;

	res = mm_map_init();
	if (res == res_FAIL) while (1) ;

	if (screen_enableBuf() == res_FAIL) while (1) hal_hw_hlt();

	if (intr_init() == res_FAIL) while (1) hal_hw_hlt();

	if (hal_hw_uefi_loadTbl() == res_FAIL) while (1) hal_hw_hlt();

	if (hal_cpu_init() == res_FAIL) while (1) hal_hw_hlt();

	if (hal_timer_init() == res_FAIL) while (1) hal_hw_hlt();

	if (hal_cpu_enableAP() == res_FAIL) while (1) hal_hw_hlt();

	intr_unmask();

	// int i = 1 / 0;

	while (1) ;
}

void hal_init_initSMP() {

}