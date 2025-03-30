#ifndef __HAL_MM_MAP_H__
#define __HAL_MM_MAP_H__

#include <lib/dtypes.h>

// size of page table must be the same as hal_mm_pageSize

#define hal_mm_pldSize  Page_4KSize
#define hal_mm_pldShift Page_4KShift

#define hal_mm_pmdSize  Page_2MSize
#define hal_mm_pmdShift Page_2MShift

#define hal_mm_pudSize  Page_1GSize
#define hal_mm_pudShift Page_1GShift

#define hal_mm_pgdSize  (Page_1GSize * 512)
#define hal_mm_pgdShift (Page_1GShift + 9)

#define hal_mm_pldIdxMask   (Page_2MSize - Page_4KSize)
#define hal_mm_pmdIdxMask   (Page_1GSize - Page_2MSize)
#define hal_mm_pudIdxMask   (hal_mm_pgdSize - Page_1GSize)
#define hal_mm_pgdIdxMask   ((1ull << 48) - hal_mm_pgdSize)

#define hal_mm_nrPageTblEntries 512

#define HAL_MM_MAP_DBG
#define HAL_MM_MAP
#define HAL_MM_UNMAP
#define HAL_MM_GETMAP

struct hal_mm_PageTbl {
    u64 entries[hal_mm_nrPageTblEntries];
} __attribute__ ((packed));

typedef struct hal_mm_PageTbl hal_mm_PageTbl;

#define hal_mm_flushTlb() do { \
    __asm__ volatile ( \
        "movq %%cr3, %%rax      \n\t" \
        "movq %%rax, %%cr3      \n\t" \
        : : : "memory" \
    ); \
} while(0)

#define hal_mm_state_Unmap  0x0
#define hal_mm_state_Premap 0x1
#define hal_mm_state_Mapped 0x2

void hal_mm_map_dbg(u64 virt);

int hal_mm_map(u64 virt, u64 phys, u64 attr);

int hal_mm_map1G(u64 virt, u64 phys, u64 attr);

int hal_mm_unmap(u64 virt);

u64 hal_mm_getMap(u64 virt);

int hal_mm_map_syncKrl();

/// @brief clear entired page table (excepts the kernel part)
/// @param pgd physical address of pgd
/// @return succ / fail
int hal_mm_map_clrTbl(u64 pgd);
#endif