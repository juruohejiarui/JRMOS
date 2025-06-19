#ifndef __MM_DMAS_H__
#define __MM_DMAS_H__

#include <mm/desc.h>
#include <task/constant.h>
#include <mm/map.h>

#define mm_dmas_phys2Virt(physAddr) ((void *)((u64)(physAddr) + mm_dmas_addrSt))
#define mm_dmas_virt2Phys(virtAddr) ((u64)((u64)(virtAddr) - mm_dmas_addrSt))

int mm_dmas_init();

__always_inline__ int mm_dmas_map(u64 phys) {
    if (mm_getMap((u64)mm_dmas_phys2Virt(phys)) == phys) return res_SUCC;
	SpinLock_lock(&mm_map_krlTblLck);
	Atomic_inc(&mm_map_krlTblModiJiff);
	int res = hal_mm_dmas_map(phys);
	SpinLock_unlock(&mm_map_krlTblLck);
	return res;
}

#endif