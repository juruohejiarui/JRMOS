#include <hal/task/api.h>
#include <hal/interrupt/desc.h>
#include <cpu/api.h>

void hal_task_sche_switchTss(task_TaskStruct *prev, task_TaskStruct *next) {
    
	__asm__ volatile ( "movq %%fs, %0" : "=a"(prev->hal.fs) : : "memory");
	__asm__ volatile ( "movq %%gs, %0" : "=a"(prev->hal.gs) : : "memory");

    __asm__ volatile ( "movq %0, %%fs" : "=a"(next->hal.fs) : : "memory");
	__asm__ volatile ( "movq %0, %%gs" : "=a"(next->hal.gs) : : "memory");
	
	hal_intr_TSS *cpuTss = cpu_desc[next->cpuId].hal.tss, *tskTss = &next->hal.tss;
	cpuTss->rsp0 = tskTss->rsp0;
	cpuTss->rsp2 = tskTss->rsp2;
	cpuTss->ist2 = tskTss->ist2;
}

void hal_task_sche_updOtherState() {
    hal_cpu_sendIntr_allExcluSelf(hal_cpu_intr_Schedule);
}