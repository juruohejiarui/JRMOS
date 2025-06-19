#include <hal/task/syscall.h>
#include <hal/hardware/reg.h>
#include <hal/init/init.h>
#include <screen/screen.h>
#include <interrupt/api.h>
#include <lib/algorithm.h>
#include <task/api.h>
#include <cpu/api.h>
#include <mm/mm.h>
#include <mm/map.h>
#include <mm/buddy.h>

int hal_task_syscall_toUsr(void (*entry)(u64), u64 param, void *usrStk) {
	task_current->hal.usrStkTop = (u64)usrStk + task_usrStkSize;
	task_current->hal.usrRsp = (u64)usrStk + task_usrStkSize - Page_4KSize;
	
	// push r11 and rcx
	__asm__ volatile (
		"swapgs				\n\t"
		"xorq %%r11, %%r11	\n\t"
		"btsq $9, %%r11		\n\t"
		"movq %%rax, %%rsp	\n\t"
		"sysretq			\n\t"
		:
		: "a"(task_current->hal.usrRsp), "c"((u64)entry), "D"(param)
		: "memory", "r11"
	);
	return res_SUCC;
}

int hal_task_syscall_init() {
	// STAR
	hal_hw_writeMsr(0xC0000081, ((u64)hal_mm_segment_KrlCode << 32) | ((u64)(hal_mm_segment_UsrData - 0x8) << 48));
	// LSTAR
	hal_hw_writeMsr(0xC0000082, (u64)hal_task_syscall_entryKrl);
	// SFMASK
	// disable interrupt
	hal_hw_writeMsr(0xC0000084, 0x200);

	// enable systemcall
	hal_hw_writeMsr(hal_msr_IA32_EFER, hal_hw_readMsr(hal_msr_IA32_EFER) | 0x1);

	hal_cpu_setvar(hal.curKrlStk, (u64)task_union(task_current)->krlStk);

	return res_SUCC;
}