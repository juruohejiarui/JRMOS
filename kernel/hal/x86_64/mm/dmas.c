#include <lib/algorithm.h>
#include <mm/mm.h>
#include <mm/dmas.h>
#include <hal/hardware/reg.h>
#include <hal/mm/map.h>
#include <hal/mm/dmas.h>
#include <hal/hardware/uefi.h>

int hal_mm_dmas_init() {
    u64 mxAddr = 0;
    for (int i = 0; i < hal_hw_uefi_info->e820Array.entryCnt; i++) {
        mxAddr = max(mxAddr, hal_hw_uefi_info->e820Array.entry[i].addr + hal_hw_uefi_info->e820Array.entry[i].len);
    }
    mxAddr = upAlign(mxAddr, hal_mm_pgdSize);
    u64 *tbl = (u64 *)upAlign(mm_memStruct.edStruct, hal_mm_pldSize);
    mm_memStruct.edStruct += sizeof(u64) * (mxAddr >> hal_mm_pudShift);
    for (u64 i = 0; i < (mxAddr >> hal_mm_pudShift); i++) {
        tbl[i] = (i << hal_mm_pudShift) | 0x87;
    }
    u64 *pud = (u64 *)(hal_hw_getCR(3) + task_krlAddrSt);
    for (u64 i = 0; i < (mxAddr >> hal_mm_pgdShift); i++) {
        pud[i + ((mm_dmas_addrSt & hal_mm_pgdIdxMask) >> hal_mm_pgdShift)]
            = ((u64)&tbl[i * hal_mm_nrPageTblEntries] - task_krlAddrSt) | 0x7;
    }
    return res_SUCC;
}

int hal_mm_dmas_map(u64 virt) {
    return hal_mm_map1G(virt, mm_dmas_virt2Phys(virt), mm_Attr_Exist);
}