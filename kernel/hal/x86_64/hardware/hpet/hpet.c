#include <hal/hardware/hpet.h>
#include <mm/dmas.h>
#include <lib/string.h>

hal_hw_hpet_XsdtDesc *hal_hw_hpet_xsdtDesc;

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
    return res_SUCC;
}