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

	res = mm_map_init();
	if (res == res_FAIL) while (1) ;

	void *addr[10][10];
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 10; j++) addr[i][j] = mm_kmalloc(sizeof(u64) << i, mm_Attr_Shared, NULL);
	}
	void *addr2[3];
	for (int i = 0; i < 3; i++) addr2[i] = mm_kmalloc(1ul << 20, mm_Attr_Shared, NULL);
	mm_slab_debug();
	mm_buddy_debug();

	int i = 1 / 0;

	while (1) ;
}