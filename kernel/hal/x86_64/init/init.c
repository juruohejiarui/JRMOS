#include <hal/init/init.h>
#include <hal/hardware/uefi.h>
#include <init/init.h>


// this is the function called by head.S
void hal_Init_init() {
	// get the information from uefi table
	hal_hw_uefi_init();
	hal_mm_loadmap();
	mm_init();
}