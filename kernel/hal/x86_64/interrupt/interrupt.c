#include <hal/interrupt/interrupt.h>
#include <hal/interrupt/trap.h>
#include <hal/init/init.h>
#include <hal/hardware/apic.h>

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
int hal_intr_init() {
	if (hal_hw_apic_init() == res_FAIL) return res_FAIL;

    // set trap gates
    hal_intr_setTss((hal_intr_TSS *)hal_init_tss,
        (u64)hal_init_stk + 32768, (u64)hal_init_stk + 32768, (u64)hal_init_stk + 32768,
        0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 
        0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

    hal_intr_initTrapGates();

    hal_intr_setTr(10);
    
    // set intr descriptors
    return res_SUCC;
}