#include <hal/interrupt/interrupt.h>
#include <hal/interrupt/trap.h>
#include <hal/hardware/reg.h>
#include <hal/init/init.h>
#include <screen/screen.h>

void hal_intr_doDivideError(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_divide_error(0),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\t" ,errorCode, rsp, *p);
	// printk(WHITE, BLACK, "pid = %d\n", Task_current->pid);
	// Task_current->priority = Task_Priority_Trapped;
	hal_intr_unmask();
	while(1) hal_hw_hlt();
}

void hal_intr_doDebug(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_debug(1),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void hal_intr_doNMI(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_nmi(2),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void hal_intr_doInt3(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_int3(3),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void hal_intr_doOverflow(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_overflow(4),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void hal_intr_doBounds(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_bounds(5),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void hal_intr_doUndefinedOpcode(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_undefined_opcode(6),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	// printk(WHITE, BLACK, "processor : %d\n", SMP_getCurCPUIndex());
	while(1);
}

void hal_intr_doDevNotAvailable(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_device_not_available(7),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	// SIMD_clrTS();
	// SIMD_switchToCur();
	while (1);
}

void hal_intr_doDoubleFault(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	u64 *q = NULL;
	p = (u64 *)(rsp + 0x98);
	q = (u64 *)(rsp + 0xa0);
	printk(RED,BLACK,"do_double_fault(8),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx,CS:%#018lx,FLAGS:%#018lx\n",errorCode , rsp , *p , *q , *(q + 1));
	while(1);
}

void hal_intr_doCoprocessorSegmentOverrun(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_coprocessor_segment_overrun(9),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void hal_intr_doInvalidTSS(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_invalid_tss(10),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	if (errorCode & 0x01)
		printk(RED,BLACK,"The exception occurred during the delivery of an event external to the program, such as an interrupt or an exception.\n");
	if (errorCode & 0x02)
		printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
	else {
		printk(RED,BLACK,"Refers to a descriptor in the GDT or the current LDT;\n");
		if (errorCode & 0x04)
			printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
		else
			printk(RED,BLACK,"Refers to a descriptor in the GDT;\n");
	}
	printk(RED,BLACK,"Segment Selector Index:%#018lx\n",errorCode & 0xfff8);
	while(1);
}

void hal_intr_doSegmentNotPresent(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_segment_not_present(11),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);

	if(errorCode & 0x01)
		printk(RED,BLACK,"The exception occurred during delivery of an event external to the program,such as an interrupt or an earlier exception.\n");

	if(errorCode & 0x02)
		printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
	else
		printk(RED,BLACK,"Refers to a descriptor in the GDT or the current LDT;\n");

	if((errorCode & 0x02) == 0)
		if(errorCode & 0x04)
			printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
		else
			printk(RED,BLACK,"Refers to a descriptor in the current GDT;\n");

	printk(RED,BLACK,"Segment Selector Index:%#010x\n",errorCode & 0xfff8);

	while(1);
}

void hal_intr_doStackSegmentFault(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_stack_segment_fault(12),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	if (errorCode & 0x01)
		printk(RED,BLACK,"The exception occurred during the delivery of an event external to the program, such as an interrupt or an exception.\n");
	if (errorCode & 0x02)
		printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
	else {
		printk(RED,BLACK,"Refers to a descriptor in the GDT or the current LDT;\n");
		if (errorCode & 0x04)
			printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
		else
			printk(RED,BLACK,"Refers to a descriptor in the GDT;\n");
	}
	printk(RED,BLACK,"Segment Selector Index:%#018lx\n",errorCode & 0xfff8);
	while(1);
}

void hal_intr_doGeneralProtection(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_general_protection(13),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\t",errorCode , rsp , *p);
	// printk(WHITE, BLACK, "pid = %ld\n", Task_current->pid);
	if (errorCode & 0x01)
		printk(RED,BLACK,"The exception occurred during the delivery of an event external to the program, such as an interrupt or an exception.\n");
	if (errorCode & 0x02)
		printk(RED,BLACK,"Refers to a gate descriptor in the IDT;\n");
	else {
		printk(RED,BLACK,"Refers to a descriptor in the GDT or the current LDT;\n");
		if (errorCode & 0x04)
			printk(RED,BLACK,"Refers to a segment or gate descriptor in the LDT;\n");
		else
			printk(RED,BLACK,"Refers to a descriptor in the GDT;\n");
	}
	printk(RED,BLACK,"Segment Selector Index:%#018lx\n",errorCode & 0xfff8);
	// Intr_Trap_printRegs(rsp);
	while(1) hal_hw_hlt();
}

// static int _isStkGrow(u64 vAddr, u64 rsp) {
// 	if (vAddr <= Task_userStackEnd)
// 		return vAddr >= min(rsp - 32, Task_userStackEnd - Page_4KSize + 0x10) && vAddr >= Task_userStackEnd - Task_userStackSize;
// 	else return 0;
// }

void hal_intr_doPageFault(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	u64 cr2 = 0;
	__asm__ volatile("movq %%cr2, %0":"=r"(cr2)::"memory");
	p = (u64 *)(rsp + 0x98);
	// u64 pldEntry = MM_PageTable_getPldEntry(getCR3(), cr2);
	// only has attributes
	// if ((pldEntry & ~0xffful) == 0 && (pldEntry & 0xffful)) {
	// 	// map this virtual address without physics page
	// 	Page *page = MM_Buddy_alloc(0, Page_Flag_Active);
	// 	// printk(WHITE, BLACK, " Task %d allocate one page %#018lx->%#018lx\n", Task_current->pid, page->phyAddr, cr2 & ~0xffful);
	// 	MM_PageTable_map(getCR3(), cr2 & ~0xffful, page->phyAddr, pldEntry | MM_PageTable_Flag_Presented);
		
	// } else if (pldEntry & ~0xffful) {
	// 	// has been presented, fault because of the old TLB
	// 	// printk(BLACK, WHITE, "[Trap]");
	// 	// printk(WHITE, BLACK, "Task %d old TLB\n");
	// 	flushTLB();
	// } else if (_isStkGrow(cr2, ((PtReg *)rsp)->rsp)) {
	// 	Page *page = MM_Buddy_alloc(0, Page_Flag_Active);
	// 	// printk(YELLOW,BLACK,"do_page_fault(14),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx,CR2:%#018lx\t",errorCode , rsp , *p , cr2);
	// 	// printk(BLACK, WHITE, "[Trap] Task %d need one stack page %#018lx->%#018lx\n", Task_current->pid, page->phyAddr, cr2 & ~0xffful);
	// 	if (Task_current->mem->pgdPhyAddr != getCR3()) hal_hw_hlt();
	// 	MM_PageTable_map(getCR3(), cr2 & ~0xffful, page->phyAddr, 
	// 		MM_PageTable_Flag_Presented | MM_PageTable_Flag_Writable | MM_PageTable_Flag_UserPage);
	// } else {
	// 	printk(RED,BLACK,"do_page_fault(14),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx,CR2:%#018lx\t",errorCode , rsp , *p , cr2);
		// printk(WHITE, BLACK, "pid = %ld\n", Task_current->pid);
		// // blank pldEntry means the page is not mapped
		// Intr_Trap_printRegs(rsp);
		// printk(RED, BLACK, "Invalid entry : %#018lx\n", pldEntry);
		if (errorCode & 0x01)
			printk(RED,BLACK,"The page fault was caused by a non-present page.\n");
		if (errorCode & 0x02)
			printk(RED,BLACK,"The page fault was caused by a page-level protection violation.\n");
		if (errorCode & 0x04)
			printk(RED,BLACK,"The page fault was caused by a non-present page.\n");
		if (errorCode & 0x08)
			printk(RED,BLACK,"The page fault was caused by a page-level protection violation.\n");
		if (errorCode & 0x10)
			printk(RED,BLACK,"The page fault was caused by reading a reserved bit.\n");
		if (errorCode & 0x20)
			printk(RED,BLACK,"The page fault was caused by an instruction fetch.\n");
		if (errorCode & 0x40)
			printk(RED,BLACK,"The page fault was caused by reading a reserved bit.\n");
		if (errorCode & 0x80)
			printk(RED,BLACK,"The page fault was caused by an instruction fetch.\n");
		while(1) hal_hw_hlt();
	// }
}

void hal_intr_doX87FPUError(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_x87_fpu_error(16),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void hal_intr_doAlignmentCheck(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_alignment_check(17),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void hal_intr_doMachineCheck(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_machine_check(18),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void hal_intr_doSIMDError(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_simd_error(19),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	// printk(WHITE, BLACK, "task %ld on processor %d\n", Task_current->pid, SMP_getCurCPUIndex());
	// u32 mxcsr = SIMD_getMXCSR();
	// printk(WHITE, BLACK, "mxcsr:%#010x\t", mxcsr);
	// mxcsr &= ~((1ul << 6) - 1);
	// SIMD_setMXCSR(mxcsr);
	while(1) hal_hw_hlt();
}

void hal_intr_doVirtualizationError(u64 rsp, u64 errorCode) {
	u64 *p = NULL;
	p = (u64 *)(rsp + 0x98);
	printk(RED,BLACK,"do_virtualization_exception(20),ERROR_CODE:%#018lx,RSP:%#018lx,RIP:%#018lx\n",errorCode , rsp , *p);
	while(1);
}

void hal_intr_initTrapGates() {
	hal_intr_setTrapGate(hal_init_idtTbl, 0, 2, hal_intr_divideError);
	hal_intr_setTrapGate(hal_init_idtTbl, 1, 2, hal_intr_debug);
	hal_intr_setIntrGate(hal_init_idtTbl, 2, 2, hal_intr_nmi);
	hal_intr_setSystemGate(hal_init_idtTbl, 3, 2, hal_intr_int3);
	hal_intr_setSystemGate(hal_init_idtTbl, 4, 2, hal_intr_overflow);
	hal_intr_setSystemGate(hal_init_idtTbl, 5, 2, hal_intr_bounds);
	hal_intr_setTrapGate(hal_init_idtTbl, 6, 2, hal_intr_undefinedOpcode);
	hal_intr_setTrapGate(hal_init_idtTbl, 7, 2, hal_intr_devNotAvailable);
	hal_intr_setTrapGate(hal_init_idtTbl, 8, 2, hal_intr_doubleFault);
	hal_intr_setTrapGate(hal_init_idtTbl, 9, 2, hal_intr_coprocessorSegmentOverrun);
	hal_intr_setTrapGate(hal_init_idtTbl, 10, 2, hal_intr_invalidTSS);
	hal_intr_setTrapGate(hal_init_idtTbl, 11, 2, hal_intr_segmentNotPresent);
	hal_intr_setTrapGate(hal_init_idtTbl, 12, 2, hal_intr_stackSegmentFault);
	hal_intr_setTrapGate(hal_init_idtTbl, 13, 2, hal_intr_generalProtection);
	hal_intr_setTrapGate(hal_init_idtTbl, 14, 2, hal_intr_pageFault);
	// 15 reserved
	hal_intr_setTrapGate(hal_init_idtTbl, 16, 2, hal_intr_x87FPUError);
	hal_intr_setTrapGate(hal_init_idtTbl, 17, 2, hal_intr_alignmentCheck);
	hal_intr_setTrapGate(hal_init_idtTbl, 18, 2, hal_intr_machineCheck);
	hal_intr_setTrapGate(hal_init_idtTbl, 19, 2, hal_intr_simdError);
	hal_intr_setTrapGate(hal_init_idtTbl, 20, 2, hal_intr_virtualizationError);
}