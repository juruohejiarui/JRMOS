#include <hal/cpu/api.h>
#include <hal/init/init.h>
#include <hal/hardware/uefi.h>
#include <hal/hardware/reg.h>
#include <hal/timer/api.h>
#include <init/init.h>
#include <interrupt/api.h>
#include <mm/mm.h>
#include <mm/map.h>
#include <mm/buddy.h>
#include <screen/screen.h>
#include <task/api.h>
#include <task/syscall.h>

u8 hal_init_stk[task_krlStkSize] __attribute__((__section__ (".data.hal_init_stk") )) = {};

u64 hal_init_testUsr(u64 param) {
	printk(WHITE, BLACK, "U%d ", param);
	while (1) {
		task_syscall0(task_syscall_release);
		printk(RED, BLACK, "U%2d ", param);
	}
	return 0;
}
u64 hal_init_test(u64 param) {
	for (int i = 0; i < param * 1000; i++) {
		printk(WHITE, BLACK, "K%2d ", param);
		hal_hw_hlt();
	}
	return 0;
}

// this is the function called by head.S
void hal_init_init() {
	intr_mask();
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

	task_sche_init();

	if (hal_cpu_enableAP() == res_FAIL) while (1) hal_hw_hlt();

	task_initIdle();

	task_syscall_initTbl();

	if (task_syscall_init() == res_FAIL) while (1) hal_hw_hlt();

	task_sche_enable();

	intr_unmask();

	task_newTask(task_freeMgr, 0, task_attr_Builtin);

	for (int i = 0; i < cpu_num * 3; i++) task_newTask(hal_init_test, i, task_attr_Builtin); 

	for (int i = 0; i < cpu_num * 3; i++) task_newTask(hal_init_testUsr, i, task_attr_Builtin | task_attr_Usr);
	while (1) {	
		// printk(BLACK, WHITE, "#%d", task_current->cpuId);
		hal_hw_hlt();
	}
}

void hal_init_initAP() {
	if (hal_intr_initAP() == res_FAIL) while (1) hal_hw_hlt();

	task_initIdle();

	if (task_syscall_init() == res_FAIL) while (1) hal_hw_hlt();

	intr_unmask();

	hal_cpu_setvar(state, cpu_Desc_state_Active);

	printk(WHITE, BLACK, "init: cpu #%d initialized\n", task_current->cpuId);
	hal_hw_mfence();

	while (1) {
		// printk(BLACK, WHITE, "#%d", task_current->cpuId);
		hal_hw_hlt();
	}
}