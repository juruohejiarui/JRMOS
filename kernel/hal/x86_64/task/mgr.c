#include <task/api.h>
#include <task/syscall.h>
#include <lib/string.h>
#include <cpu/api.h>
#include <hal/interrupt/api.h>

void hal_task_newProc(task_Process *proc, u64 attr) {
	if (~attr & task_attr_Usr) {
		proc->mem.hal.pgd = mm_krlTblPhysAddr;
	} else {
		// fork the page table from kernel table
		hal_mm_PageTbl *pgd = mm_map_allocTbl();

		memcpy(mm_dmas_phys2Virt(mm_krlTblPhysAddr), pgd, sizeof(hal_mm_PageTbl));
		memset(pgd, 0, sizeof(hal_mm_PageTbl) / 2);
		proc->mem.hal.pgd = mm_dmas_virt2Phys(pgd);
	}
	
}

void hal_task_tskEntry(void *entry, u64 arg, u64 attr) {
	if (~attr & task_attr_Usr) {
		u64 res = ((u64 (*)(u64))entry)(arg);
		task_exit(res);
	} else {
		/// @todo allocation of user stack
		hal_task_syscall_toUsr(entry, arg);
	}
}

int hal_task_freeThread(task_Process *proc) {
	// clear map table
	if (hal_mm_map_clrTbl(proc->mem.hal.pgd) == res_FAIL) return res_FAIL;
	return res_SUCC;
}

int hal_task_freeThd(task_Thread *tsk) {
	return res_SUCC;
}

void hal_task_exit(u64 res) {
}

void hal_task_newThd(task_Thread *thd, void *entryAddr, void *arg, u64 attr) {
	hal_intr_PtReg *ptReg = (hal_intr_PtReg *)((container(thd, task_Union, thd))->krlStk + task_krlStkSize - sizeof(hal_intr_PtReg));
	memset(ptReg, 0, sizeof(hal_intr_PtReg));
	
	ptReg->cs = hal_mm_segment_KrlCode;
	ptReg->ds = hal_mm_segment_KrlData;
	ptReg->es = hal_mm_segment_KrlData;
	ptReg->ss = hal_mm_segment_KrlData;

	thd->hal.fs = thd->hal.gs = hal_mm_segment_KrlData;

	ptReg->rsp = (u64)((task_Union *)thd)->krlStk + task_krlStkSize;
	ptReg->rdi = (u64)entryAddr;
	ptReg->rsi = (u64)arg;
	ptReg->rdx = attr;
	
	ptReg->rflags = (1ull<< 9);
	ptReg->rip = (u64)hal_task_tskEntry;

	thd->hal.rflags = 0;
	thd->hal.rip = (u64)hal_intr_retFromIntr;
	thd->hal.rsp = (u64)ptReg;

	{
		register u64 intrRsp = (u64)((task_Union *)thd)->krlStk + task_krlStkSize;
		hal_intr_setTss(&thd->hal.tss, 
			intrRsp, intrRsp, intrRsp, intrRsp, intrRsp,
			intrRsp, intrRsp, intrRsp, intrRsp, intrRsp);
	}
}

void hal_task_initIdleThd() {
	memcpy(cpu_ptr(hal_intr_tss)->tss, &task_cur->hal.tss, sizeof(hal_intr_TSS));
	
	task_cur->hal.fs = task_cur->hal.gs = hal_mm_segment_KrlData;

	task_cur->hal.gsBase = cpu_getvar(cpu_bsAddr);
	task_cur->hal.fsBase = 0;
	task_cur->hal.gsKrlBase = 0;

	task_cur->hal.rsp = (u64)cpu_getvar(hal_cpu_initStk) + task_krlStkSize;
}