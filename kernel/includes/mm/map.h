#ifndef __MM_MAP_H__
#define __MM_MAP_H__

#include <lib/dtypes.h>

int mm_map(u64 phys, u64 virt, u8 size, u64 attr);

int mm_unmap(u64 phys, u64 virt, u8 size, u64 attr);

#endif