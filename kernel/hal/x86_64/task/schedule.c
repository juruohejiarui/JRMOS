#include <hal/task/api.h>
#include <hal/interrupt/desc.h>
#include <hal/interrupt/api.h>
#include <hal/task/syscall.h>
#include <hal/hardware/reg.h>
#include <mm/map.h>
#include <mm/dmas.h>
#include <task/api.h>
#include <cpu/api.h>
#include <lib/string.h>
#include <screen/screen.h>

int hal_task_dispatchTask(task_TaskStruct *tsk) {
	tsk->cpuId = tsk->pid % cpu_num;
	
	tsk->hal.gsBase = (u64)cpu_getCpuVar(tsk->cpuId, cpu_bsAddr);
	tsk->hal.fsBase = 0;
	tsk->hal.gsKrlBase = 0;

	return res_SUCC;
}

void hal_task_sche_yield() {
	hal_cpu_sendIntr_self(hal_cpu_intr_Schedule);
}

void hal_task_sche_switchTss(task_TaskStruct *prev, task_TaskStruct *next) {
	{
		register hal_intr_TSS *cpuTss = cpu_ptr(hal_intr_tss)->tss, *tskTss = &next->hal.tss;
		cpuTss->rsp0 = tskTss->rsp0;
		cpuTss->rsp2 = tskTss->rsp2;
		cpuTss->ist2 = tskTss->ist2;
		// printk(screen_log, "tss:%p<%p\n", cpuTss, tskTss);
	}

	register u64 gsBase = cpu_getvar(cpu_bsAddr);

	__asm__ volatile ( "movq %%fs, %0" : "=a"(prev->hal.fs));
	__asm__ volatile ( "movq %%gs, %0" : "=a"(prev->hal.gs));

    __asm__ volatile ( "movq %0, %%fs" : : "a"(next->hal.fs));
	__asm__ volatile ( "movq %0, %%gs" : : "a"(next->hal.gs));

	hal_hw_writeMsr(hal_msr_IA32_GS_BASE, gsBase);
}

void hal_task_sche_updOtherState() {
    hal_cpu_sendIntr_allExcluSelf(hal_cpu_intr_Schedule);
}

void hal_task_newThread(task_ThreadStruct *thread, u64 attr) {
	if (~attr & task_attr_Usr) {
		thread->mem.hal.pgd = mm_krlTblPhysAddr;
	} else {
		// fork the page table from kernel table
		hal_mm_PageTbl *pgd = mm_map_allocTbl();

		memcpy(mm_dmas_phys2Virt(mm_krlTblPhysAddr), pgd, sizeof(hal_mm_PageTbl));
		memset(pgd, 0, sizeof(hal_mm_PageTbl) / 2);
		thread->mem.hal.pgd = mm_dmas_virt2Phys(pgd);
	}
	
}

void hal_task_initIdle() {
	memcpy(cpu_ptr(hal_intr_tss)->tss, &task_cur->hal.tss, sizeof(hal_intr_TSS));

	task_cur->hal.fs = task_cur->hal.gs = hal_mm_segment_KrlData;

	task_cur->hal.gsBase = cpu_getvar(cpu_bsAddr);
	task_cur->hal.fsBase = 0;
	task_cur->hal.gsKrlBase = 0;
    task_cur->hal.rsp = (u64)cpu_getvar(hal_cpu_initStk) + task_krlStkSize;
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

int hal_task_freeThread(task_ThreadStruct *thread) {
	// clear map table
	if (hal_mm_map_clrTbl(thread->mem.hal.pgd) == res_FAIL) return res_FAIL;
	return res_SUCC;
}

int hal_task_freeTask(task_TaskStruct *tsk) {
	return res_SUCC;
}

void hal_task_exit(u64 res) {
}

void hal_task_newSubTask(task_TaskStruct *tsk, void *entryAddr, u64 arg, u64 attr) {
	hal_intr_PtReg *ptReg = (hal_intr_PtReg *)(((task_Union *)tsk)->krlStk + task_krlStkSize - sizeof(hal_intr_PtReg));
	memset(ptReg, 0, sizeof(hal_intr_PtReg));
	
	ptReg->cs = hal_mm_segment_KrlCode;
	ptReg->ds = hal_mm_segment_KrlData;
	ptReg->es = hal_mm_segment_KrlData;
	ptReg->ss = hal_mm_segment_KrlData;

	tsk->hal.fs = tsk->hal.gs = hal_mm_segment_KrlData;

	ptReg->rsp = (u64)((task_Union *)tsk)->krlStk + task_krlStkSize;
	ptReg->rdi = (u64)entryAddr;
	ptReg->rsi = arg;
	ptReg->rdx = attr;
	
	ptReg->rflags = (1ull<< 9);
	ptReg->rip = (u64)hal_task_tskEntry;

	tsk->hal.rflags = 0;
	tsk->hal.rip = (u64)hal_intr_retFromIntr;
	tsk->hal.rsp = (u64)ptReg;

	{
		register u64 intrRsp = (u64)((task_Union *)tsk)->krlStk + task_krlStkSize;
		hal_intr_setTss(&tsk->hal.tss, 
			intrRsp, intrRsp, intrRsp, intrRsp, intrRsp,
			intrRsp, intrRsp, intrRsp, intrRsp, intrRsp);
	}
}

void hal_task_newTask(task_TaskStruct *tsk, void *entryAddr, u64 arg, u64 attr) {
	hal_task_newSubTask(tsk, entryAddr, arg, attr);
}