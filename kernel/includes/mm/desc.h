#ifndef __MM_DESC_H__
#define __MM_DESC_H__

#include <lib/list.h>
#include <task/constant.h>
#include <hal/mm/mm.h>

#define mm_Attr_Shared      0x1u
#define mm_Attr_Shared2U    0x2u
#define mm_Attr_Firmware    0x4u
#define mm_Attr_HeadPage	0x8u
#define mm_Attr_Allocated	0x10u
#define mm_Attr_Coverage	0x20u

#define mm_kernelAddr	((void *)(task_krlAddrSt + 0x100000ul))

#ifndef hal_mm_pageSize
#define mm_pageSize		hal_mm_pageSize
#define mm_pageShift	hal_mm_pageShift
#else
#error no definition of hal_mm_pageSize and hal_mm_pageShift for this arch!
#endif

extern char mm_symbol_text;
extern char mm_symbol_etext;
extern char mm_symbol_rodata;
extern char mm_symbol_erodata;
extern char mm_symbol_data;
extern char mm_symbol_edata;
extern char mm_symbol_end;

struct mm_Page {
	List list;
	u64 physAddr;
	u32 ord;
	u32 attr;
} __attribute__ ((packed));

struct mm_MemMap {
    u64 addr;
    u64 size;
    u32 attr;
} __attribute__ ((packed));

typedef struct mm_MemMap mm_MemMap;

typedef struct mm_Page mm_Page;

typedef struct mm_Zone {
	u64 phyAddrSt, phyAddrEd;
	mm_Page *page;
	u64 pageLen;
	u64 availSt;
	u64 totFree, totUsing;
} mm_Zone;

#define mm_maxNrMemMapEntries 32
extern u32 mm_nrMemMapEntries;
extern mm_MemMap mm_memMapEntries[mm_maxNrMemMapEntries];

typedef struct mm_MemStruct {
	mm_Zone zones[mm_maxNrMemMapEntries];
	u64 nrZones;
	mm_Page *pages;
	u64 nrPages;
	u64 totMem;
} mm_MemStruct;

extern mm_MemStruct mm_memStruct; 
 
#endif