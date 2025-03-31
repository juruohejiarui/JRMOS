#include <mm/mm.h>
#include <mm/dmas.h>
#include <mm/buddy.h>
#include <lib/algorithm.h>
#include <lib/bit.h>
#include <lib/spinlock.h>
#include <lib/string.h>
#include <screen/screen.h>
#include <interrupt/api.h>

static struct BuddyStruct {
    u64 *revBit[mm_buddy_mxOrd + 1];
    List freeLst[mm_buddy_mxOrd + 1];
    u64 tot, totUsage;
} _buddy;

static SpinLock _buddyLck;

static __always_inline__ int _getBit(mm_Page *page) {
    if (page->buddyId == 1) return 1;
    u64 bitId = (mm_getPhyAddr(page) >> (mm_pageShift + page->ord + 1));
    return (_buddy.revBit[page->ord][bitId / sizeof(u64)] >> (bitId % sizeof(u64))) & 1;
}

static __always_inline__ void _revBit(mm_Page *page) {
    if (page->buddyId == 1) return ;
    u64 bitId = (mm_getPhyAddr(page) >> (mm_pageShift + page->ord + 1));
    // printk(WHITE, BLACK, "_revBit(): page:%#081lx ord=%d, bitId=%#018lx\n", mm_getPhyAddr(page), page->ord, bitId);
    bit_rev(&_buddy.revBit[page->ord][bitId / sizeof(u64)], (bitId % sizeof(u64)));
}

static __always_inline__ mm_Page *_getBuddy(mm_Page *page) {
    return mm_buddy_isRight(page) ? page - (1ull<< page->ord) : page + (1ull<< page->ord);
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
    }
    _buddy.tot = _buddy.totUsage = 0;
    for (int i = 0; i <= mm_buddy_mxOrd; i++)
        List_init(&_buddy.freeLst[i]);
    for (int i = 0; i < mm_memStruct.nrZones; i++) {
        mm_Zone *zone = &mm_memStruct.zones[i];
        for (u64 addr = zone->availSt; addr < zone->phyAddrEd; ) {
            u32 ord = addr ? min(mm_buddy_mxOrd, bit_ffs64(addr) - mm_pageShift - 1) : mm_buddy_mxOrd;
            while (addr + (1ull<< (ord + mm_pageShift)) > zone->phyAddrEd) ord--;
            mm_Page *page = mm_getDesc(addr);
            page->buddyId = 1;
            page->attr |= mm_Attr_HeadPage;
            page->ord = ord;
            List_insBefore(&page->list, &_buddy.freeLst[ord]);
            addr += (1ull<< (ord + mm_pageShift));
            _buddy.tot += (1ull<< (ord + mm_pageShift));
        }
    }
    
    return res_SUCC;
}

void mm_buddy_debug(int detail) {
    printk(WHITE, BLACK, "mm: buddy: usage %ldB=%ldMb/%ldB=%ldMb ", _buddy.totUsage, _buddy.totUsage >> 20, _buddy.tot, _buddy.tot >> 20);
    if (!detail)
        return ;
    printk(WHITE, BLACK, "\n");
    for (int i = 0; i <= mm_buddy_mxOrd; i++) {
        printk(YELLOW, BLACK, "[%2d] ", i);
        for (List *list = _buddy.freeLst[i].next; list != &_buddy.freeLst[i]; list = list->next) {
            mm_Page *page = container(list, mm_Page, list);
            printk(WHITE, BLACK, "%#018x,", mm_getPhyAddr(page));
        }
        printk(WHITE, BLACK, "\n");
    }
}

mm_Page *mm_allocPages(u64 log2Size, u32 attr) {
    // printk(WHITE, BLACK, "mm: buddy: alloc 2^%ld page attr=%#010x\n", log2Size, attr);
    int intrState = intr_state();
    intr_mask();
    SpinLock_lock(&_buddyLck);
    mm_Page *resPage = NULL;
    if (log2Size > mm_buddy_mxOrd) goto Fail;

    for (int ord = log2Size; ord <= mm_buddy_mxOrd; ord++) {
        if (List_isEmpty(&_buddy.freeLst[ord])) continue;
        resPage = container(_buddy.freeLst[ord].next, mm_Page, list);
        resPage->ord = ord;
        break;
    }
    if (resPage == NULL) goto Fail;

    List_del(&resPage->list);
    _revBit(resPage);
    for (int i = resPage->ord; i > log2Size; i--) {
        mm_Page *rPage = mm_divPageGrp(resPage);
        rPage->attr |= mm_Attr_HeadPage;
        rPage->ord = i - 1;
        _revBit(rPage);
        List_insBefore(&rPage->list, &_buddy.freeLst[i - 1]);
    }
    _buddy.totUsage += (1ull<< (log2Size + mm_pageShift));
    SpinLock_unlock(&_buddyLck);
    if (intrState) intr_unmask();
    resPage->attr = attr | mm_Attr_HeadPage | mm_Attr_Allocated;
    return resPage;
    Fail:
    
    SpinLock_unlock(&_buddyLck);
    if (intrState) intr_unmask();
    printk(RED, BLACK, "mm: buddy: failed to allocate page group with size=2^%d,attr=%#010x\n", log2Size, attr);
    return NULL;
}

int mm_freePages(mm_Page *pages) {
    if (~pages->attr & (mm_Attr_HeadPage | mm_Attr_Allocated)) return res_FAIL;
    
    int intrState = intr_state();
    intr_mask();
    SpinLock_lock(&_buddyLck);
    _buddy.totUsage -= (1ull<< (pages->ord + mm_pageShift));
    pages->attr &= ~mm_Attr_Allocated;
    for (int i = pages->ord; i <= mm_buddy_mxOrd; i++) {
        _revBit(pages);
        if (_getBit(pages)) break;
        mm_Page *bud = _getBuddy(pages);
        List_del(&bud->list);
        if (mm_buddy_isRight(pages)) {
            pages->buddyId = pages->ord = 0;
            pages->attr &= ~mm_Attr_HeadPage;
            pages = bud;
        }
        pages->buddyId = mm_buddy_parent(pages->buddyId);
        pages->ord++;
    }
    List_insBehind(&pages->list, &_buddy.freeLst[pages->ord]);
    SpinLock_unlock(&_buddyLck);
    if (intrState) intr_unmask();

    return res_SUCC;
}