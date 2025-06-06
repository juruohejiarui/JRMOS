#ifndef __MM_DESC_H__
#define __MM_DESC_H__

#include <lib/list.h>
#include <lib/rbtree.h>
#include <task/constant.h>
#include <hal/mm/mm.h>
#include <hal/mm/dmas.h>

#define mm_Attr_Shared      0x1u
#define mm_Attr_Shared2U    0x2u
#define mm_Attr_Firmware    0x4u
#define mm_Attr_HeadPage	0x8u
#define mm_Attr_Allocated	0x10u
#define mm_Attr_MMU			0x20u
#define mm_Attr_System		0x40u
#define mm_Attr_Exist		0x80u
#define mm_Attr_Writable	0x100u

#define mm_kernelAddr	((void *)(task_krlAddrSt + 0x100000ul))

#ifdef HAL_MM_PAGESIZE
#define mm_pageSize		hal_mm_pageSize
#define mm_pageShift	hal_mm_pageShift
#else
#error no definition of hal_mm_pageSize and hal_mm_pageSize for this arch!
#endif

#ifdef HAL_MM_KRLTBLPHYSADDR
#define mm_krlTblPhysAddr	hal_mm_krlTblPhysAddr
#else
#error no definition of mm_krlTblPhysAddr for this arch!
#endif

#define mm_dmas_addrSt	(0xffff880000000000ul)

#ifdef HAL_MM_DMAS_MAPSIZE
#define mm_dmas_mapSize		hal_mm_dmas_mapSize
#else
#error no definition of mm_dmas_mapSize	for this arch!
#endif

#define mm_usrAddrSt	(0x0ul)

extern char mm_symbol_text;
extern char mm_symbol_etext;
extern char mm_symbol_rodata;
extern char mm_symbol_erodata;
extern char mm_symbol_data;
extern char mm_symbol_edata;
extern char mm_symbol_end;

struct mm_Page {
	ListNode list;
	u16 buddyId;
	u16 ord;
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
	mm_Page *page;
	u64 phyAddrSt, phyAddrEd;
	u64 pageLen;
	u64 availSt;
	u64 totFree, totUsing;
} mm_Zone;

#define mm_maxNrMemMapEntries 32
extern u32 mm_nrMemMapEntries;
extern mm_MemMap mm_memMapEntries[mm_maxNrMemMapEntries];

typedef struct mm_MemStruct {
	mm_Zone zones[mm_maxNrMemMapEntries];
	u32 nrZones, krlZoneId;
	u64 totMem;
	u64 edStruct;
} mm_MemStruct;

#define mm_slab_mnSizeShift	5
#define mm_slab_mnSize		(1ull<< mm_slab_mnSizeShift)
#define mm_slab_mxSizeShift	20
#define mm_slab_mxSize		(1ull<< mm_slab_mxSizeShift)

typedef struct mm_SlabRecord {
	void *ptr;
	u64 size;
	RBNode rbNode;
	void (*destructor)(void *ptr);
} mm_SlabRecord;

extern mm_MemStruct mm_memStruct; 

#endif