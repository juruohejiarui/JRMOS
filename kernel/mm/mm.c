#include <mm/mm.h>
#include <mm/dmas.h>
#include <mm/buddy.h>
#include <hal/mm/mm.h>
#include <screen/screen.h>
#include <lib/algorithm.h>

u32 mm_nrMemMapEntries;
mm_MemMap mm_memMapEntries[mm_maxNrMemMapEntries];
mm_MemStruct mm_memStruct;

int mm_init() {
    hal_mm_loadmap();
    int krlZoneId = -1, lstRam = 0;
    mm_memStruct.nrZones = 0;
    for (int i = 0; i < mm_nrMemMapEntries; i++) {
        mm_MemMap *entry = &mm_memMapEntries[i];
        if (entry->attr & mm_Attr_Firmware) continue;

        mm_Zone *zone = &mm_memStruct.zones[mm_memStruct.nrZones];

        zone->phyAddrSt = entry->addr;
        zone->phyAddrEd = entry->addr + entry->size;

        if (entry->addr + entry->size <= (u64)mm_kernelAddr - task_krlAddrSt) {
            zone->availSt = zone->phyAddrEd;
        } else zone->availSt = zone->phyAddrSt;
        
        // get the kernel zone index
        if (entry->addr <= (u64)mm_kernelAddr - task_krlAddrSt && (u64)&mm_symbol_end - task_krlAddrSt < entry->addr + entry->size)
            mm_memStruct.krlZoneId = mm_memStruct.nrZones;
        
        mm_memStruct.nrZones++;
    }
    mm_memStruct.edStruct = (u64)&mm_symbol_end;

    mm_dmas_init();

    mm_memStruct.zones[mm_memStruct.krlZoneId].availSt = upAlign(mm_memStruct.edStruct - task_krlAddrSt, mm_pageSize);
    // from now on, printk can be used
    printk(WHITE, BLACK, "mm: edStruct: %#018lx mm_symbol_end: %#018lx kernel zone id:%d\nmm:", mm_memStruct.edStruct, &mm_symbol_end, mm_memStruct.krlZoneId);

    u64 pgDescArrSize;
    {
        u64 mxAddr = 0;
        for (int i = 0; i < mm_nrMemMapEntries; i++)
            mxAddr = max(mxAddr, mm_memMapEntries[i].addr + mm_memMapEntries[i].size);
        pgDescArrSize = (upAlign(mxAddr, mm_pageSize) >> mm_pageShift) * sizeof(mm_Page);
        pgDescArrSize = upAlign(pgDescArrSize, mm_pageSize);
    }
    // find a valid space for page array
    {
        mm_Zone *tgrZone = NULL;
        for (int i = 0; i < mm_memStruct.nrZones; i++) {
            mm_Zone *zone = &mm_memStruct.zones[i];
            if (zone->availSt + pgDescArrSize <= zone->phyAddrEd) {
                tgrZone = zone;
                break;
            }
        }
        if (tgrZone == NULL) {
            printk(RED, BLACK, "mm: failed to find valid space for page descriptor array.");
            return res_FAIL;
        }
        mm_memStruct.pages = mm_dmas_phys2Virt(tgrZone->availSt);
        tgrZone->availSt += pgDescArrSize;
        printk(WHITE, BLACK, "mm: page array: %#018lx~%#018lx\n", mm_memStruct.pages, tgrZone->availSt);
    }
    // modify the zone information, align the address space to 4K, write the pointer of page arrary, initialize usage information
    for (int i = 0; i < mm_memStruct.nrZones; i++) {
        mm_Zone *zone = &mm_memStruct.zones[i];
        zone->availSt = upAlign(zone->availSt, mm_pageSize);
        zone->phyAddrSt = upAlign(zone->phyAddrSt, mm_pageSize);
        zone->phyAddrEd = downAlign(zone->phyAddrEd, mm_pageSize);
        zone->totFree = zone->phyAddrEd - zone->availSt;
        zone->totUsing = 0;

        zone->page = mm_getDesc(zone->phyAddrSt);
        zone->pageLen = (zone->phyAddrEd - zone->phyAddrSt) >> mm_pageShift;
    }
    printk(WHITE, BLACK, "mm:");
    for (int i = 0; i < mm_memStruct.nrZones; i++) {
        printk(YELLOW, BLACK, "\t[%2d]:addr:[%#018lx,%#018lx],avail:%#018lx\n", i, mm_memStruct.zones[i].phyAddrSt, mm_memStruct.zones[i].phyAddrEd, mm_memStruct.zones[i].availSt);
    }

    // calculate the total available RAM
    mm_memStruct.totMem = 0;
    for (int i = 0; i < mm_memStruct.nrZones; i++)
        mm_memStruct.totMem += mm_memStruct.zones[i].totFree;
    printk(WHITE, BLACK, "mm: totMem:%#018lx=%ldMB\n", mm_memStruct.totMem, mm_memStruct.totMem >> 10);

    if (mm_buddy_init() == res_FAIL) return res_FAIL;
    if (mm_slab_init() == res_FAIL) return res_FAIL;

    return res_SUCC;
}

mm_Page *mm_divPageGrp(mm_Page *grpHdr) {
    if (!grpHdr->ord) return NULL;
    mm_Page *rPart = grpHdr + (1ull<< (--grpHdr->ord));
    rPart->ord = grpHdr->ord;
    rPart->attr = grpHdr->attr;
    rPart->buddyId = mm_buddy_rson(grpHdr->buddyId);
    grpHdr->buddyId = mm_buddy_lson(grpHdr->buddyId);
    return rPart;
}

mm_Page *mm_init_allocPage(u64 num, u32 attr) {
    num <<= mm_pageShift;
    for (int i = 0; i < mm_memStruct.nrZones; i++) {
        if (mm_memStruct.zones[i].totFree >= num) {
            mm_Page *page = mm_getDesc(mm_memStruct.zones[i].availSt + mm_memStruct.zones[i].totUsing);
            mm_memStruct.zones[i].totUsing += num;
            mm_memStruct.zones[i].totFree -= num;
            page->attr = attr | mm_Attr_Allocated | mm_Attr_HeadPage | mm_Attr_System;
            return page;
        }
    }
    return NULL;
}