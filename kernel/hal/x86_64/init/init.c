#include <hal/init/init.h>
#include <hal/hardware/uefi.h>
#include <init/init.h>
#include <mm/mm.h>
#include <mm/map.h>
#include <mm/buddy.h>

u8 hal_init_stk[task_krlStkSize] __attribute__((__section__ (".data.hal_init_stack") )) = {};

// this is the function called by head.S
void hal_init_init() {
	// get the information from uefi table
	hal_hw_uefi_init();

	screen_init();
	
	int res = mm_init();
	if (res == res_FAIL) while (1) ;

	res = mm_map_initCache();
	if (res == res_FAIL) while (1) ;

	printk(WHITE, BLACK, "res: %#018lx * %#018lx\n", hal_hw_uefi_info->screenInfo.pixelPreLine, hal_hw_uefi_info->screenInfo.verRes);
	mm_buddy_debug();
	mm_Page *pages1 = mm_allocPages(4, mm_Attr_Shared), *pages2;
	// mm_buddy_debug();
	pages2 = mm_allocPages(2, mm_Attr_Shared);
	mm_buddy_debug();
	printk(YELLOW, BLACK, "pages1:%#018lx, pages2:%#018lx\n", mm_getPhyAddr(pages1), mm_getPhyAddr(pages2));
	// mm_freePages(pages1);
	// mm_freePages(pages2);
	mm_buddy_debug();

	int i = 1 / 0;

	while (1) ;
}