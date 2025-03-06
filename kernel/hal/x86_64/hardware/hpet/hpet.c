#include <hal/hardware/hpet.h>
#include <hal/hardware/io.h>
#include <hal/hardware/reg.h>
#include <interrupt/desc.h>
#include <interrupt/api.h>
#include <mm/dmas.h>
#include <lib/string.h>

hal_hw_hpet_XsdtDesc *hal_hw_hpet_xsdtDesc;

intr_Ctrl hal_hw_hpet_intrCtrl;
intr_Info hal_hw_hpet_intrInfo;

intr_handlerDeclare(hal_hw_hpet_intrHandler) {
    task_sche_updState();
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
    printk(WHITE, BLACK, "hw: hpet: desc:%#018lx\n", hal_hw_hpet_xsdtDesc);

    hal_hw_io_out32(0xcf8, 0x8000f8f0);
    u32 x = hal_hw_io_in32(0xcfc) & 0xffffc000;
    if (x > 0xfec00000 && x < 0xfee00000) {
        printk(WHITE, BLACK, "hw: hpet: set enable register.");
        u32 *p = (u32 *)mm_dmas_phys2Virt(x + 0x3404ul);
        *p = 0x80;
        hal_hw_mfence();
    } else printk(WHITE, BLACK, "hw: hpet: no need to set enable register.\n");
    
    
    return res_SUCC;
}