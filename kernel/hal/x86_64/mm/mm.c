#include <hal/mm/mm.h>
#include <mm/mm.h>
#include <hal/hardware/uefi.h>

void hal_mm_loadmap() {
    mm_nrMemMapEntries = hal_hw_uefi_info->e820Array.entryCnt;
    for (int i = 0; i < mm_nrMemMapEntries; i++) {
        mm_memMapEntries[i].addr = hal_hw_uefi_info->e820Array.entry[i].addr;
        mm_memMapEntries[i].size = hal_hw_uefi_info->e820Array.entry[i].len;
        mm_memMapEntries[i].attr = 0;
        if (hal_hw_uefi_info->e820Array.entry[i].tp != 1)
            mm_memMapEntries[i].attr |= mm_Attr_Firmware;
    }
}