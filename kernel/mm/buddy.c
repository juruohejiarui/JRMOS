#include <mm/mm.h>
#include <mm/dmas.h>
#include <mm/buddy.h>
#include <lib/algorithm.h>
#include <lib/bit.h>
#include <lib/spinlock.h>
#include <lib/string.h>
#include <screen/screen.h>
#include <interrupt/interrupt.h>

static struct BuddyStruct {
    u64 *revBit[mm_buddy_mxOrd + 1];
    List freeLst[mm_buddy_mxOrd + 1];
} _buddy;

static SpinLock _buddyLck;

#define _isRight(page) ((page)->buddyId & 1)
#define _leftSon(buddyId) ((buddyId) << 1)
#define _rightSon(buddyId) ((buddyId) << 1 | 1)

static __always_inline__ int _getBit(mm_Page *page) {
    u64 bitId = (mm_getPhyAddr(page) >> (mm_pageShift + page->ord)) - _isRight(page);
    return (_buddy.revBit[page->ord][bitId / sizeof(u64)] >> (bitId % sizeof(u64))) & 1;
}

static __always_inline__ void _revBit(mm_Page *page) {
    u64 bitId = (mm_getPhyAddr(page) >> (mm_pageShift + page->ord)) - _isRight(page);
    bit_rev(&_buddy.revBit[page->ord][bitId / sizeof(u64)], (bitId % sizeof(u64)));
}

static __always_inline__ mm_Page *_getBuddy(mm_Page *page) {
    return _isRight(page) ? page - (1ul << page->ord) : page + (1ul << page->ord);
}

int mm_buddy_init() {
    SpinLock_init(&_buddyLck);
    u64 mxAddr = 0;
    for (int i = 0; i < mm_memStruct.nrZones; i++)
        mxAddr = max(mxAddr, mm_memStruct.zones[i].phyAddrEd);
    for (int i = 0; i <= mm_buddy_mxOrd; i++) {
        u64 ali = upAlign((upAlign(mxAddr, mm_pageSize << i) >> (mm_pageShift + i)), 64) / 8;
        u64 nrPage = upAlign(ali, mm_pageSize) >> mm_pageShift;
        mm_Page *pages = mm_init_allocPage(nrPage, 0);
        if (pages == NULL) {
            printk(RED, BLACK, "mm: buddy: failed to allocate bitmap #%d.\n", i);
            return res_FAIL;
        }
        _buddy.revBit[i] = mm_dmas_phys2Virt(mm_getPhyAddr(pages));
        memset(_buddy.revBit[i], 0, nrPage << mm_pageShift);

        printk(WHITE, BLACK, "mm: buddy: bitmap #%d: %#018lx, size=%#018lx\n", i, _buddy.revBit[i], ali);
    }

    for (int i = 0; i <= mm_buddy_mxOrd; i++) {
        List_init(&_buddy.freeLst[i]);
        mm_Zone *zone = &mm_memStruct.zones[i];
        for (u64 addr = zone->availSt; addr < zone->phyAddrEd; ) {
            u32 ord = addr ? min(mm_buddy_mxOrd, bit_ffs64(addr) - mm_pageShift - 1) : mm_buddy_mxOrd;
            while (addr + (1ul << (ord + mm_pageShift)) > zone->phyAddrEd) ord--;
            mm_Page *page = mm_getDesc(addr);
            page->buddyId = 1;
            page->attr |= mm_Attr_HeadPage;
            
        }
    }
    
    return res_SUCC;
}