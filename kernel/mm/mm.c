#include <mm/mm.h>
#include <mm/dmas.h>
#include <hal/mm/mm.h>
#include <screen/screen.h>

u32 mm_nrMemMapEntries;
mm_MemMap mm_memMapEntries[mm_maxNrMemMapEntries];
mm_MemStruct mm_memStruct;

void mm_init() {
    hal_mm_loadmap();
    int krlZoneId = -1, lstRam = 0;
    mm_memStruct.nrZones = 0;
    for (int i = 0; i < mm_nrMemMapEntries; i++) {
        mm_MemMap *entry = &mm_memMapEntries[i];
        if (entry->attr & mm_Attr_Firmware) continue;
        mm_memStruct.nrZones++;
        
        if (entry->addr + entry->size <= (u64)mm_kernelAddr - task_krlAddrSt) continue;
        if (entry->addr <= (u64)mm_kernelAddr - task_krlAddrSt && (u64)&mm_symbol_end - task_krlAddrSt < entry->addr + entry->size)
            mm_memStruct.nrZones = mm_memStruct.nrZones;
    }
    mm_memStruct.edStruct = (u64)&mm_symbol_end;

    mm_dmas_init();
    // from now on, printk can be used
    printk(WHITE, BLACK, "mm: edStruct: %#018lx\n", mm_memStruct.edStruct);
    for (int i = 0; i < mm_nrMemMapEntries; i++) {
        printk(YELLOW, BLACK, "[%2d]: addr:%#018lx len:%#018lx attr:%#010lx\n", i, mm_memMapEntries[i].addr, mm_memMapEntries[i].size, mm_memMapEntries[i].attr);
    }
}
mm_Page *mm_divPageGrp(mm_Page *grpHdr) {
    if (!grpHdr->ord) return NULL;
    mm_Page *rPart = grpHdr + (1ul << (--grpHdr->ord));
    rPart->ord = grpHdr->ord;
    rPart->attr = grpHdr->attr;
    return rPart;
}