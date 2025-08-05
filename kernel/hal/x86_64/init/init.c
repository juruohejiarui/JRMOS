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
#include <softirq/api.h>
#include <init/init.h>
#include <lib/algorithm.h>
#include <lib/hash.h>

u8 hal_init_stk[task_krlStkSize] __attribute__((__section__ (".data.hal_init_stk") )) = {};

u64 hal_init_testUsr_recur(u64 dep, u64 lim);

u64 hal_init_testUsr(u64 param) {
	hal_init_testUsr_recur(0, min(Page_4KSize * param, Page_4KSize * 8));
	task_syscall1(task_syscall_exit, 0);
	return 0;
}

u64 hal_init_testUsr_recur(u64 dep, u64 lim) {
	// hal_hw_hlt();
	if (dep < lim) hal_init_testUsr_recur(dep + 1, lim);
	else {
		// printu(WHITE, BLACK, "U%ld ", dep);
		// task_syscall1(task_syscall_exit, 0);
	}
	return 0;
}

u64 hal_init_test(u64 param) {
	task_signal_setHandler(task_Signal_Int, task_exit, -1);
	for (int i = 0; i < param * 100; i++) {
		if (i == 11 * param) task_signal_send(task_current, task_Signal_Int);
		task_sche_yield();
	}
	return 0;
}

// this is the function called by head.S
void hal_init_init() {
	intr_mask();
	
	hash_init();

	// get the information from uefi table
	hal_hw_uefi_init();

	screen_init();
	
	int res = mm_init();
	if (res & res_FAIL) while (1) ;

	res = mm_map_init();
	if (res == res_FAIL) {
		printk(screen_err, "init: map init failed\n");
		while (1) ;
	}

	if (screen_enableBuf() == res_FAIL) {
		printk(screen_err, "init: screen buf init failed\n");
		while (1) hal_hw_hlt();
	}


	if (intr_init() == res_FAIL) {
		printk(screen_err, "init: intr init failed\n");
		while (1) hal_hw_hlt();
	}
	if (softirq_init() == res_FAIL) while (1) hal_hw_hlt();

	if (hal_hw_uefi_loadTbl() == res_FAIL) while (1) hal_hw_hlt();

	if (hal_cpu_init() == res_FAIL) while (1) hal_hw_hlt();

	if (hal_timer_init() == res_FAIL) while (1) hal_hw_hlt();

	task_sche_init();

	task_syscall_initTbl();

	if (hal_cpu_enableAP() == res_FAIL) while (1) hal_hw_hlt();

	task_initIdle();

	task_signal_init();

	hal_cpu_chk();

	if (task_syscall_init() == res_FAIL) while (1) hal_hw_hlt();

	task_sche_enable();

	intr_unmask();

	task_sche_freeMgrTsk = task_newSubTask(task_freeMgr, 0, task_attr_Builtin);

	init_init();

	// for (int i = 0; i < 3; i++) task_newTask(hal_init_testUsr, i, task_attr_Builtin | task_attr_Usr);

	while (1) {
		hal_hw_hlt();
	}
}

void hal_init_initAP() {
	hal_hw_writeMsr(hal_msr_IA32_GS_BASE, (u64)&cpu_desc[task_current->cpuId]);
	if (hal_intr_initAP() == res_FAIL) while (1) hal_hw_hlt();

	task_initIdle();

	if (task_syscall_init() == res_FAIL) while (1) hal_hw_hlt();

	intr_unmask();

	hal_cpu_setvar(state, cpu_Desc_state_Active);

	hal_cpu_chk();

	// printk(screen_log, "init: cpu #%d initialized\n", task_current->cpuId);
	hal_hw_mfence();

	while (1) {
		hal_hw_hlt();
	}
}