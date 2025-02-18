#ifndef __TASK_CONSTANT_H__
#define __TASK_CONSTANT_H__

#include <lib/dtypes.h>

#define Task_krlStkSize	(32768ul)
// 8 MB
#define Task_UsrStkSize	(1ull << 23)

#define Task_krlAddrSt	(0xffff800000000000ul)
#define Task_dmasAddrSt	(0xffff880000000000ul)

#endif