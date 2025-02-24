#include <lib/algorithm.h>
#include <hal/mm/map.h>
#include <hal/mm/dmas.h>
#include <hal/hardware/uefi.h>

int hal_mm_dmas_init() {
    u64 mxAddr = 0;
    for (int i = 0; i < hal_hw_uefi_info->e820Array.entryCnt; i++) {
        mxAddr = max(mxAddr, hal_hw_uefi_info->e820Array.entry[i].addr + hal_hw_uefi_info->e820Array.entry[i].len);
    }
    mxAddr = upAlign(mxAddr, hal_mm_pgdSize) >> hal_mm_pudShift;
    
}