#include <hal/task/syscall.h>
#include <hal/hardware/reg.h>
#include <screen/screen.h>
#include <task/api.h>
#include <mm/mm.h>
#include <mm/map.h>
#include <mm/buddy.h>

int hal_task_syscall_toUsr(void (*entry)(u64), u64 param, void *usrStk) {
	// map one page to the user stack
	task_UsrStruct *usr = NULL;
	{
		mm_Page *pg = mm_allocPages(0, mm_Attr_Shared2U);
		if (!pg)
			return res_FAIL;
		printk(WHITE, BLACK, "allocate page %#018lx\n", mm_getPhyAddr(pg));
		if (mm_map((u64)usrStk, mm_getPhyAddr(pg), mm_Attr_Shared2U | mm_Attr_Exist | mm_Attr_Writable) == res_FAIL)
			return res_FAIL;
		mm_dbg((u64)usrStk);
		printk(WHITE, BLACK, "finish map %#018lx->%#018lx\n", mm_getPhyAddr(pg), usrStk);
		usr = (task_UsrStruct *)usrStk;
	}

	usr->tsk = task_current;
	usr->hal.krlFs = usr->hal.krlGs = hal_mm_segment_KrlData;
	usr->hal.usrFs = usr->hal.usrGs = hal_mm_segment_UsrData;

	// minuss 16 bytes for the return address and the rbp
	task_current->hal.rsp2 = (u64)usrStk + task_usrStkSize - sizeof(u64) * 2;
	task_current->hal.usrStkTop = (u64)usrStk + task_usrStkSize;

	// push r11 and rcx
	__asm__ volatile (
		"cli				\n\t"
		"pushq %0			\n\t"
		"xor %%rax, %%rax	\n\t"
		"pushq %%rax		\n\t"
		"leaq hal_task_syscall_backUsr(%%rip), %%rax	\n\t"
		"jmpq *%%rax		\n\t"
		:
		: "b"(entry), "D"(param)
		: "memory"
	);
}

int hal_task_syscall_init() {
	// STAR
	hal_hw_writeMsr(0xC0000081, ((u64)hal_mm_segment_KrlCode << 32) | ((u64)hal_mm_segment_UsrData << 48));
	// LSTAR
	hal_hw_writeMsr(0xC0000082, (u64)hal_task_syscall_entryKrl);
	// SFMASK
	// disable interrupt
	hal_hw_writeMsr(0xC0000084, 0x200);
}