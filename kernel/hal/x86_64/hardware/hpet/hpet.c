#include <hal/hardware/hpet.h>
#include <hal/hardware/apic.h>
#include <hal/hardware/io.h>
#include <hal/hardware/reg.h>
#include <hal/cpu/api.h>
#include <interrupt/desc.h>
#include <interrupt/api.h>
#include <mm/dmas.h>
#include <lib/string.h>
#include <task/api.h>
#include <timer/api.h>

hal_hw_hpet_XsdtDesc *hal_hw_hpet_xsdtDesc;
hal_hw_apic_RteDesc hal_hw_hpet_rteDesc;

intr_handlerDeclare(hal_hw_hpet_intrHandler) {
    task_sche_updState();
    timer_updSirq();
    // hal_cpu_sendIntr_allExcluSelf(cpu_intr_Pause);
}

int hal_hw_hpet_init() {
    hal_hw_hpet_xsdtDesc = NULL;
    for (u64 i = 0; i < hal_hw_uefi_xsdtTbl->hdr.length; i++) {
        hal_hw_uefi_AcpiHeader *hdr = mm_dmas_phys2Virt(hal_hw_uefi_xsdtTbl->entry[i]);
        if (!strncmp(hdr->signature, "HPET", 4)) {
            hal_hw_hpet_xsdtDesc = container(hdr, hal_hw_hpet_XsdtDesc, hdr);
            break;
        }
    }
    if (hal_hw_hpet_xsdtDesc == NULL) {
        printk(RED, BLACK, "hw: hpet: cannot find HPET configuration in UEFI Table.\n");
        return res_FAIL;
    }
    printk(WHITE, BLACK, "hw: hpet: desc:%#018lx addr:%#018lx\n", hal_hw_hpet_xsdtDesc, hal_hw_hpet_xsdtDesc->addr.addr);

    hal_hw_io_out32(0xcf8, 0x8000f8f0);
    u32 x = hal_hw_io_in32(0xcfc) & 0xffffc000;
    if (x > 0xfec00000 && x < 0xfee00000) {
        printk(WHITE, BLACK, "hw: hpet: set enable register.\n");
        u32 *p = (u32 *)mm_dmas_phys2Virt(x + 0x3404ul);
        *p = 0x80;
        hal_hw_mfence();
    } else printk(WHITE, BLACK, "hw: hpet: no need to set enable register.\n");

    memset(&hal_hw_hpet_rteDesc, 0, sizeof(hal_hw_apic_RteDesc));
    
    hal_hw_hpet_rteDesc.vector = hal_intr_vec_Timer;
    hal_hw_hpet_rteDesc.deliveryMode = hal_hw_apic_DeliveryMode_Fixed;
    hal_hw_hpet_rteDesc.destMode = hal_hw_apic_DestMode_Physical;
    hal_hw_hpet_rteDesc.deliveryStatus = hal_hw_apic_DeliveryStatus_Idle;
    hal_hw_hpet_rteDesc.pinPolarity = hal_hw_apic_PinPolarity_High;
    hal_hw_hpet_rteDesc.remoteIRR = hal_hw_apic_RemoteIRR_Reset;
    hal_hw_hpet_rteDesc.triggerMode = hal_hw_apic_TriggerMode_Edge;
    hal_hw_hpet_rteDesc.destDesc.logical.dest = hal_cpu_bspApicId;

    // check capability
    u64 cap = hal_hw_hpet_getReg64(hal_hw_hpet_capId);
    printk(WHITE, BLACK, "hw: hpet: cap: ");
    if (cap & hal_hw_hpet_capId_64bit) printk(GREEN, BLACK, "64bit ");
    else {
        printk(RED, BLACK, "64bit ");
        return res_FAIL;
    }
    if (cap & hal_hw_hpet_capId_LegacyReplace) printk(GREEN, BLACK, "Legacy-Replace ");
    else {
        printk(RED, BLACK, "Legacy-Replace ");
        return res_FAIL;
    }

    // get the minimum tick of HPET
    u64 minTick = (cap >> 32) & ((1ull<< 32) - 1);
    printk(WHITE, BLACK, "minTick=%ld\n", minTick);

    // set and check check configuration and capability of timer 0
    hal_hw_setTimerCfg(0, hal_hw_hpet_timerCfgCap_Enable | hal_hw_hpet_timerCfgCap_Period | hal_hw_hpet_timerCfgCap_SetVal | hal_hw_hpet_timerCfgCap_Irq(2));
    u64 cfgCap0 = hal_hw_getTimerCfg(0);
    printk(WHITE, BLACK, "hw: hpet: timer0: ");
    if (cfgCap0 & hal_hw_hpet_timerCfgCap_Enable) printk(GREEN, BLACK, "Enable ");
    else {
        printk(RED, BLACK, "Disable ");
        return res_FAIL;
    }
    if (cfgCap0 & hal_hw_hpet_timerCfgCap_64Cap) printk(GREEN, BLACK, "64bit ");
    else {
        printk(RED, BLACK, "64bit ");
        return res_FAIL;
    }
    if (cfgCap0 & hal_hw_hpet_timerCfgCap_Period) printk(GREEN, BLACK, "Period ");
    else {
        printk(RED, BLACK, "Period ");
        return res_FAIL;
    }
    printk(WHITE, BLACK, "tick=%ld\n", ((1000000000000) / minTick));
    hal_hw_setTimerCmp(0, ((1000000000000) / minTick));

    intr_initDesc(hal_hw_hpet_intrDesc, hal_hw_hpet_intrHandler, 0, "HPET", &hal_hw_apic_intrCtrl);

    hal_hw_hpet_intrDesc->vecId = hal_intr_vec_Timer;
    hal_hw_hpet_intrDesc->cpuId = cpu_bspIdx;

    hal_hw_hpet_setReg64(hal_hw_hpet_mainCounter, 0x0);
    hal_hw_hpet_setReg64(hal_hw_hpet_intrState, 0xfffffffful);
    hal_hw_hpet_setReg64(hal_hw_hpet_cfg, hal_hw_hpet_cfg_Enable | hal_hw_hpet_cfg_LegacyReplace);
    printk(WHITE, BLACK, "hw: hpet: cfg: ");
    {
        u64 cfg = hal_hw_hpet_getReg64(hal_hw_hpet_cfg);
        if (cfg & hal_hw_hpet_cfg_Enable) printk(GREEN, BLACK, "Enable ");
        else {
            printk(RED, BLACK, "Disable ");
            return res_FAIL;
        }
        if (cfg & hal_hw_hpet_cfg_LegacyReplace) printk(GREEN, BLACK, "Legacy-Replace\n");
        else {
            printk(RED, BLACK, "Legacy-Replace\n");
            return res_FAIL;
        }
    }

    if (intr_install(hal_hw_hpet_intrDesc, &hal_hw_hpet_rteDesc) == res_FAIL) return res_FAIL;
    if (intr_enable(hal_hw_hpet_intrDesc) == res_FAIL) return res_FAIL;
    return res_SUCC;
}

int hal_hw_hpet_enable() {
    intr_enable(hal_hw_hpet_intrDesc);
}