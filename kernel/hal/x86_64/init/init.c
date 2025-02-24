#include <hal/init/init.h>
#include <hal/hardware/uefi.h>
#include <init/init.h>
#include <mm/mm.h>

u8 hal_init_stk[task_krlStkSize] __attribute__((__section__ (".data.Init_stack") )) = {};

// this is the function called by head.S
void hal_init_init() {
	// get the information from uefi table
	hal_hw_uefi_init();
	
	mm_init();
}