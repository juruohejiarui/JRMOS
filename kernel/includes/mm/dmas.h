#ifndef __MM_DMAS_H__
#define __MM_DMAS_H__

#include <task/constant.h>

#define mm_dmas_phys2Virt(physAddr) ((void *)((physAddr) + Task_dmasAddrSt))
#define mm_dmas_virt2Phys(virtAddr) ((u64)((virtAddr) - Task_dmasAddrSt))
#endif