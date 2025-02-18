#include <hal/init/init.h>
#include <hal/hardware/uefi.h>
#include <init/init.h>
#include <mm/mm.h>


// this is the function called by head.S
void hal_Init_init() {
	// get the information from uefi table
	hal_hw_uefi_init();
	mm_init();
}