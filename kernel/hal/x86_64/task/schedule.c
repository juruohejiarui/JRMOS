#include <hal/task/api.h>
#include <hal/interrupt/desc.h>
#include <hal/interrupt/api.h>
#include <mm/map.h>
#include <mm/dmas.h>
#include <cpu/api.h>
#include <lib/string.h>

int hal_task_dispatchTask(task_TaskStruct *tsk) {
	return tsk->pid % cpu_num;
}

void hal_task_sche_switchTss(task_TaskStruct *prev, task_TaskStruct *next) {
	hal_intr_TSS *cpuTss = cpu_desc[next->cpuId].hal.tss, *tskTss = &next->hal.tss;
	cpuTss->rsp0 = tskTss->rsp0;
	cpuTss->rsp2 = tskTss->rsp2;
	cpuTss->ist2 = tskTss->ist2;

	__asm__ volatile ( "movq %%fs, %0" : "=a"(prev->hal.fs));
	__asm__ volatile ( "movq %%gs, %0" : "=a"(prev->hal.gs));

    __asm__ volatile ( "movq %0, %%fs" : : "a"(next->hal.fs));
	__asm__ volatile ( "movq %0, %%gs" : : "a"(next->hal.gs));
}

void hal_task_sche_updOtherState() {
    hal_cpu_sendIntr_allExcluSelf(hal_cpu_intr_Schedule);
}

void hal_task_newThread(hal_task_ThreadStruct *thread) {
	// fork the page table from kernel table
	hal_mm_PageTbl *pgd = mm_map_allocTbl();

	memcpy(mm_dmas_phys2Virt(mm_krlTblPhysAddr), pgd, sizeof(hal_mm_PageTbl));
	thread->pgd = mm_dmas_virt2Phys(pgd);
}

void hal_task_initIdle() {
	cpu_Desc *curCpu = &cpu_desc[hal_task_current->cpuId];
	memcpy(curCpu->hal.tss, &hal_task_current->hal.tss, sizeof(hal_intr_TSS));

	hal_task_current->hal.fs = hal_task_current->hal.gs = hal_mm_segment_KrlData;
    hal_task_current->hal.rsp = (u64)curCpu->hal.initStk + task_krlStkSize;
}

void hal_task_newSubTask(task_TaskStruct *tsk, void *entryAddr, u64 arg, u64 attr) {
	hal_intr_PtReg *ptReg = (hal_intr_PtReg *)(((task_Union *)tsk)->krlStk + task_krlStkSize - sizeof(hal_intr_PtReg));
	memset(ptReg, 0, sizeof(hal_intr_PtReg));
	if (attr & task_attr_Usr) {
		ptReg->cs = hal_mm_segment_UsrCode;
		ptReg->ds = hal_mm_segment_UsrData;
		ptReg->es = hal_mm_segment_UsrData;
		ptReg->ss = hal_mm_segment_UsrData;

		/// @todo allocate user level stack
	} else {
		ptReg->cs = hal_mm_segment_KrlCode;
		ptReg->ds = hal_mm_segment_KrlData;
		ptReg->es = hal_mm_segment_KrlData;
		ptReg->ss = hal_mm_segment_KrlData;

		ptReg->rsp = (u64)((task_Union *)tsk)->krlStk + task_krlStkSize;
	}
	
	ptReg->rdi = arg;
	ptReg->rip = (u64)entryAddr;

	tsk->hal.fs = tsk->hal.gs = hal_mm_segment_UsrData;
	tsk->hal.rflags = 0;
	tsk->hal.rip = (u64)hal_intr_retFromIntr;
	tsk->hal.rsp = ptReg->rsp;
	hal_intr_setTss(&tsk->hal.tss, 
		ptReg->rsp, ptReg->rsp, ptReg->rsp, ptReg->rsp, ptReg->rsp, 
		0, 0, 0, 0, 0);
}