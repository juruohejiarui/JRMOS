#include <lib/algorithm.h>
#include <mm/mm.h>
#include <mm/dmas.h>
#include <hal/hardware/reg.h>
#include <hal/mm/map.h>
#include <hal/mm/dmas.h>
#include <hal/hardware/uefi.h>
#include <screen/screen.h>

int hal_mm_dmas_init() {
    u64 mxAddr = 0;
    for (int i = 0; i < hal_hw_uefi_info->e820Array.entryCnt; i++) {
        mxAddr = max(mxAddr, hal_hw_uefi_info->e820Array.entry[i].addr + hal_hw_uefi_info->e820Array.entry[i].len);
    }
    mxAddr = upAlign(mxAddr, hal_mm_pgdSize);
    u64 *tbl = (u64 *)upAlign(mm_memStruct.edStruct, hal_mm_pldSize);
    mm_memStruct.edStruct = (u64)tbl + sizeof(u64) * (mxAddr >> hal_mm_pudShift);
    for (u64 i = 0; i < (mxAddr >> hal_mm_pudShift); i++) {
        tbl[i] = (i << hal_mm_pudShift) | 0x83;
    }
    u64 *pgd = (u64 *)(hal_hw_getCR(3) + task_krlAddrSt);
    for (u64 i = 0; i < (mxAddr >> hal_mm_pgdShift); i++) {
        pgd[i + ((mm_dmas_addrSt & hal_mm_pgdIdxMask) >> hal_mm_pgdShift)]
            = ((u64)&tbl[i * hal_mm_nrPageTblEntries] - task_krlAddrSt) | 0x3;
    }
    hal_mm_flushTlb();
    return res_SUCC;
}

int hal_mm_dmas_map(u64 phys) {
    phys &= ~(mm_dmas_mapSize - 1);
    return hal_mm_map1G((u64)mm_dmas_phys2Virt(phys), phys, mm_Attr_Exist | mm_Attr_Writable);
}