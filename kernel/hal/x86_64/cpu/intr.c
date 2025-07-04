#include <hal/cpu/api.h>
#include <hal/init/init.h>
#include <hal/hardware/apic.h>
#include <interrupt/api.h>
#include <screen/screen.h>
#include <task/api.h>

static intr_Desc hal_cpu_intrDesc[0x10];
static intr_Ctrl hal_cpu_intrCtrl;

void hal_cpu_intrDispatcher(u64 rsp, u64 irqId) {
	intr_Desc *desc = &hal_cpu_intrDesc[irqId - 0x80];
	if (desc->handler) desc->handler(desc->param);
	else {
		printk(screen_err, "cpu: no interrupt handler for irq %#x\n", irqId);
	}
	desc->ctrl->ack(desc);
}

#define buildIrq(x) hal_intr_buildIrq(hal_cpu_intr, x, hal_cpu_intrDispatcher)

buildIrq(hal_cpu_intr_Schedule);
buildIrq(hal_cpu_intr_Pause);
buildIrq(hal_cpu_intr_Execute);

void (*hal_cpu_irqList[0x10])(void) = {
	hal_cpu_intr0x80, hal_cpu_intr0x81, hal_cpu_intr0x82
};

intr_handlerDeclare(hal_cpu_intrOfSchedule) {
	task_sche_updCurState();
}

intr_handlerDeclare(hal_cpu_intrOfPause) {
	printk(screen_err, "P%d ", task_current->cpuId);
}

int hal_cpu_initIntr() {
	for (int i = 0; i < 0x10; i++) hal_intr_setIntrGate(hal_init_idtTbl, i + 0x80, 0, hal_cpu_irqList[i]);
	
	memset(&hal_cpu_intrCtrl, 0, sizeof(intr_Ctrl));
	hal_cpu_intrCtrl.ack = hal_hw_apic_edgeAck;

	{
		intr_Desc *desc = &hal_cpu_intrDesc[0];
		memset(desc, 0, sizeof(intr_Desc));
		desc->handler = hal_cpu_intrOfSchedule;
		desc->ctrl = &hal_cpu_intrCtrl;
		desc->name = "cpu schedule";
	}

	{
		intr_Desc *desc = &hal_cpu_intrDesc[1];
		memset(desc, 0, sizeof(intr_Desc));
		desc->handler = hal_cpu_intrOfPause;
		desc->ctrl = &hal_cpu_intrCtrl;
		desc->name = "cpu pause";
	}

	return res_SUCC; 
}

void hal_cpu_sendIntr(u64 irq, u32 cpuId) {
	hal_hw_apic_IcrDesc icr;
	*(u64 *)&icr = hal_hw_apic_makeIcr32(irq, hal_hw_apic_DestShorthand_None);

	hal_hw_apic_setIcrDest(&icr, cpu_desc[cpuId].hal.apic, cpu_desc[cpuId].hal.x2apic);

	hal_hw_apic_writeIcr(*(u64 *)&icr);
}