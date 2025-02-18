#include <mm/mm.h>
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
        
    }
}