#include <hal/interrupt/api.h>
#include <hal/interrupt/trap.h>
#include <hal/init/init.h>
#include <hal/hardware/apic.h>
#include <interrupt/desc.h>
#include <screen/screen.h>
#include <cpu/desc.h>
#include <task/api.h>

cpu_definevar(hal_intr_TssBlk, hal_intr_tss);
cpu_definevar(hal_intr_IdtBlk, hal_intr_idt);
cpu_definevar(intr_Desc *[0x40], hal_intr_desc);

void hal_intr_setTss(hal_intr_TSS * tss,
    u64 rsp0, u64 rsp1, u64 rsp2, u64 ist1, u64 ist2, u64 ist3, u64 ist4, u64 ist5, u64 ist6, u64 ist7) {
    tss->rsp0 = rsp0;
    tss->rsp1 = rsp1;
    tss->rsp2 = rsp2;

    tss->ist1 = ist1;
    tss->ist2 = ist2;
    tss->ist3 = ist3;
    tss->ist4 = ist4;
    tss->ist5 = ist5;
    tss->ist6 = ist6;
    tss->ist7 = ist7;

    tss->iomapBaseAddr = 0;
}
void hal_intr_setTssItem(u64 idx, hal_intr_TSS *tss) {
    u64 lmt = 103;
    *(u64 *)(hal_init_gdtTbl + idx) = 
            (lmt & 0xffff)
            | (((u64)tss & 0xffff) << 16)
            | (((u64)tss >> 16 & 0xff) << 32)
            | ((u64)0x89 << 40)
            | ((lmt >> 16 & 0xf) << 48)
            | (((u64)tss >> 24 & 0xff) << 56);
    *(u64 *)(hal_init_gdtTbl + idx + 1) = ((u64)tss >> 32 & 0xffffffff) | 0;
}

void hal_intr_dispatcher(u64 rsp, u64 irqId) {
    // printk(screen_warn, "intr: cpu:%d irq:%x\t", cpu_getvar(cpu_id), irqId);
    intr_Desc *intr = &hal_intr_desc[irqId - 0x20];
    if (__unlikely__(intr->handler == NULL)) {
        printk(screen_err, "intr: no handler for intr #%#lx\n", irqId);
    } else intr->handler(intr->param);
    if (intr->ctrl != NULL && intr->ctrl->ack != NULL) intr->ctrl->ack(intr);
}

#define buildIrq(x) hal_intr_buildIrq(hal_intr_irq, x, hal_intr_dispatcher)

buildIrq(0x20)  buildIrq(0x21)  buildIrq(0x22)  buildIrq(0x23)
buildIrq(0x24)  buildIrq(0x25)  buildIrq(0x26)  buildIrq(0x27)
buildIrq(0x28)  buildIrq(0x29)  buildIrq(0x2a)  buildIrq(0x2b)
buildIrq(0x2c)  buildIrq(0x2d)  buildIrq(0x2e)  buildIrq(0x2f)
buildIrq(0x30)  buildIrq(0x31)  buildIrq(0x32)  buildIrq(0x33)
buildIrq(0x34)  buildIrq(0x35)  buildIrq(0x36)  buildIrq(0x37)

void (*hal_intr_entryList[24])(void) = {
    hal_intr_irq0x20, hal_intr_irq0x21, hal_intr_irq0x22, hal_intr_irq0x23, 
    hal_intr_irq0x24, hal_intr_irq0x25, hal_intr_irq0x26, hal_intr_irq0x27,
    hal_intr_irq0x28, hal_intr_irq0x29, hal_intr_irq0x2a, hal_intr_irq0x2b,
    hal_intr_irq0x2c, hal_intr_irq0x2d, hal_intr_irq0x2e, hal_intr_irq0x2f,
    hal_intr_irq0x30, hal_intr_irq0x31, hal_intr_irq0x32, hal_intr_irq0x33,
    hal_intr_irq0x34, hal_intr_irq0x35, hal_intr_irq0x36, hal_intr_irq0x37
};

intr_Desc hal_intr_desc[24];

void hal_intr_setInCpuDesc(intr_Desc *desc, u32 intrNum) {
    for (int i = 0; i < intrNum; i++) cpu_setCpuVar(desc[i].cpuId, hal_intr_desc[desc[i].vecId - 0x40], &desc[i]);
}

int hal_intr_init() {
	if (hal_hw_apic_init() == res_FAIL) return res_FAIL;

    // set trap gates
    hal_intr_setTss((hal_intr_TSS *)hal_init_tss,
        (u64)hal_init_stk + task_krlStkSize, (u64)hal_init_stk + task_krlStkSize, (u64)hal_init_stk + task_krlStkSize,
        0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 
        0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

    hal_intr_initTrapGates();

    hal_intr_setTr(10);
    
    // set intr gates
    for (int i = 0; i < 24; i++) hal_intr_setIntrGate(hal_init_idtTbl, i + 0x20, 0, hal_intr_entryList[i]);
    
    return res_SUCC;
}

int hal_intr_initCpuVar(int idx) {
    memset(cpu_cpuPtr(idx, intr_msk[0]), 0xff, sizeof(cpu_var(intr_msk)));
    cpu_setCpuVar(idx, intr_msk[1], 0);
    SpinLock_init(cpu_cpuPtr(idx, intr_mskLck));
    cpu_setCpuVar(idx, intr_freeCnt, 64);
    cpu_setCpuVar(idx, intr_useCnt, 3 * 64);
}
int hal_intr_initAP() {
    hal_intr_setTr(cpu_ptr(hal_intr_tss)->trIdx);
    
    if (hal_hw_apic_initAP() == res_FAIL) return res_FAIL;
    return res_SUCC;
}