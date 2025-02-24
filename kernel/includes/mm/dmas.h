#ifndef __MM_DMAS_H__
#define __MM_DMAS_H__

#include <mm/desc.h>
#include <task/constant.h>

#define mm_dmas_addrSt	(0xffff880000000000ul)

#define mm_dmas_phys2Virt(physAddr) ((void *)((physAddr) + mm_dmas_addrSt))
#define mm_dmas_virt2Phys(virtAddr) ((u64)((virtAddr) - mm_dmas_addrSt))

int mm_dmas_init();

#endif