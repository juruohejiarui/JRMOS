#include <hal/task/schedule.h>
#include <task/api.h>
#include <hal/cpu/desc.h>
#include <hal/hardware/reg.h>
#include <hal/interrupt/desc.h>
#include <cpu/api.h>
#include <screen/screen.h>

int hal_task_sche_dispatch(task_Thread *tsk) {
	tsk->cpuId = tsk->pid % cpu_num;
	
	tsk->hal.gsBase = (u64)cpu_getCpuVar(tsk->cpuId, cpu_bsAddr);
	tsk->hal.fsBase = 0;
	tsk->hal.gsKrlBase = 0;

	return res_SUCC;
}

__optimize__ void hal_task_sche_switchTss(task_Thread *prev, task_Thread *next) {
	{
		register hal_intr_TSS *cpuTss = cpu_ptr(hal_intr_tss)->tss, *tskTss = &next->hal.tss;
		cpuTss->rsp0 = tskTss->rsp0;
		cpuTss->rsp2 = tskTss->rsp2;
		cpuTss->ist2 = tskTss->ist2;
		// printk(screen_log, "tss:%p<%p\n", cpuTss, tskTss);
	}

	prev->hal.gsBase = hal_hw_readMsr(hal_msr_IA32_GS_BASE);
	prev->hal.fsBase = hal_hw_readMsr(hal_msr_IA32_FS_BASE);
	prev->hal.gsKrlBase = hal_hw_readMsr(hal_msr_IA32_KERNEL_GS_BASE);

	// printk(screen_log, "%p->hal:gs:%#018lx,fs:%#018lx %p->hal.gs:%#018lx,fs:%#018lx\n",
		// prev, prev->hal.fs, prev->hal.gs, next, next->hal.fs, next->hal.gs);

	__asm__ volatile ( "movq %%fs, %0" : "=a"(prev->hal.fs));
	__asm__ volatile ( "movq %%gs, %0" : "=a"(prev->hal.gs));

    __asm__ volatile ( "movq %0, %%fs" : : "a"(next->hal.fs));
	__asm__ volatile ( "movq %0, %%gs" : : "a"(next->hal.gs));

	hal_hw_writeMsr(hal_msr_IA32_GS_BASE, next->hal.gsBase);
	hal_hw_writeMsr(hal_msr_IA32_FS_BASE, next->hal.fsBase);
	hal_hw_writeMsr(hal_msr_IA32_KERNEL_GS_BASE, next->hal.gsKrlBase);

	cpu_setvar(task_curThd, next);
}