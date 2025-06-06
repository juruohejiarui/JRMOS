#include <mm/mm.h>
#include <mm/dmas.h>
#include <mm/buddy.h>
#include <hal/mm/mm.h>
#include <screen/screen.h>
#include <lib/string.h>
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
        if (entry->addr <= (u64)mm_kernelAddr - task_krlAddrSt && (u64)&mm_symbol_end - task_krlAddrSt <= entry->addr + entry->size)
            mm_memStruct.krlZoneId = mm_memStruct.nrZones;
        
        mm_memStruct.nrZones++;
    }
    mm_memStruct.edStruct = (u64)&mm_symbol_end;
    mm_dmas_init();

    mm_memStruct.zones[mm_memStruct.krlZoneId].availSt = upAlign(mm_memStruct.edStruct - task_krlAddrSt, mm_pageSize);
    // from now on, printk can be used
    printk(WHITE, BLACK, "mm: edStruct: %p mm_symbol_end: %p kernel zone id:%d\nmm:", mm_memStruct.edStruct, &mm_symbol_end, mm_memStruct.krlZoneId);

    // print layout
    for (int i = 0; i < mm_nrMemMapEntries; i++) {
        mm_MemMap *entry = &mm_memMapEntries[i];
        if (entry->attr & mm_Attr_Firmware) continue;
        printk(WHITE, BLACK, "\t%d: %lx %lx\n", i, entry->addr, entry->size);
    }
    // modify the zone information, align the address space to 4K, write the pointer of page arrary, initialize usage information
    for (int i = 0; i < mm_memStruct.nrZones; i++) {
        mm_Zone *zone = &mm_memStruct.zones[i];
        zone->availSt = upAlign(zone->availSt, mm_pageSize);
        zone->phyAddrSt = upAlign(zone->phyAddrSt, mm_pageSize);
        zone->phyAddrEd = downAlign(zone->phyAddrEd, mm_pageSize);
        zone->totFree = zone->phyAddrEd - zone->availSt;
        zone->totUsing = 0;

        zone->pageLen = (zone->phyAddrEd - zone->phyAddrSt) >> mm_pageShift;
    }
    printk(WHITE, BLACK, "mm:");
    for (int i = 0; i < mm_memStruct.nrZones; i++) {
        printk(YELLOW, BLACK, "\t[%2d]:addr:[%p,%p],avail:%p free:%lx\n", i, mm_memStruct.zones[i].phyAddrSt, mm_memStruct.zones[i].phyAddrEd, mm_memStruct.zones[i].availSt, mm_memStruct.zones[i].totFree);
    }

    // find space for each zone to place page descriptor array
    for (int i = 0; i < mm_memStruct.nrZones; i++) {
        mm_Zone *zone = &mm_memStruct.zones[i], *tgrZone = NULL;
        u64 pgDescArrSz = upAlign((zone->phyAddrEd - zone->phyAddrSt) / Page_4KSize * sizeof(mm_Page), Page_4KSize);
        for (int j = 0; j < mm_memStruct.nrZones; j++) {
            mm_Zone *zone2 = &mm_memStruct.zones[j];
            if (zone2->totFree >= pgDescArrSz) {
                tgrZone = zone2;
                break;
            }
        } 
        if (tgrZone == NULL) {
            printk(RED, BLACK, "failed to allocate page descriptor array for zone %d: size:%#lx\n", i, pgDescArrSz);
            while (1) ;
        }
        zone->page = mm_dmas_phys2Virt(zone->availSt + zone->totUsing);
        zone->totUsing += pgDescArrSz;
        tgrZone->totFree -= pgDescArrSz;
    }

    // calculate the total available RAM
    mm_memStruct.totMem = 0;
    for (int i = 0; i < mm_memStruct.nrZones; i++)
        mm_memStruct.totMem += mm_memStruct.zones[i].totFree;
    printk(WHITE, BLACK, "mm: totMem:%p=%ldMB\n", mm_memStruct.totMem, mm_memStruct.totMem >> 10);

    if (mm_buddy_init() == res_FAIL) return res_FAIL;
    if (mm_slab_init() == res_FAIL) return res_FAIL;

    return res_SUCC;
}

mm_Page *mm_divPageGrp(mm_Page *grpHdr) {
    if (!grpHdr->ord) return NULL;
    --grpHdr->ord;
    mm_Page *rPart = (mm_Page *)((void *)grpHdr + (1ull << grpHdr->ord) * sizeof(mm_Page));
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

void mm_dbg() {
    mm_buddy_dbg(0);
    mm_slab_dbg(0);
    printk(WHITE, BLACK, "\r");
}

mm_Page *mm_getDesc(u64 phyAddr) {
    for (int i = 0; i < mm_memStruct.nrZones; i++) {
        mm_Zone *zone = &mm_memStruct.zones[i];
        if (zone->phyAddrEd <= phyAddr) continue;
        if (zone->phyAddrSt > phyAddr) break;
        return zone->page + ((phyAddr - zone->phyAddrSt) >> Page_4KShift);
    }
    return NULL;
}

u64 mm_getPhyAddr(mm_Page *desc) {
    for (int i = 0; i < mm_memStruct.nrZones; i++) {
        mm_Zone *zone = &mm_memStruct.zones[i];
        mm_Page *lstPage = zone->page + ((zone->phyAddrEd - zone->phyAddrSt) >> Page_4KShift);
        if (zone->page <= desc && desc < lstPage)
            return zone->phyAddrSt + ((desc - zone->page) << Page_4KShift);
    }
    return (u64)NULL;
}