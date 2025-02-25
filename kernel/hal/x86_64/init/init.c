#include <hal/init/init.h>
#include <hal/hardware/uefi.h>
#include <init/init.h>
#include <mm/mm.h>

u8 hal_init_stk[task_krlStkSize] __attribute__((__section__ (".data.hal_init_stack") )) = {};

// this is the function called by head.S
void hal_init_init() {
	// get the information from uefi table
	hal_hw_uefi_init();

	screen_init();
	
	int res = mm_init();

	printk(WHITE, BLACK, "res: %#018lx * %#018lx\n", hal_hw_uefi_info->screenInfo.pixelPreLine, hal_hw_uefi_info->screenInfo.verRes);

	int i = 1 / 0;

	while (1) ;
}