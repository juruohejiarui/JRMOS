#ifndef __TASK_STRUCTS_H__
#define __TASK_STRUCTS_H__

#include <task/constant.h>
#include <lib/list.h>
#include <lib/rbtree.h>
#include <hal/task/structs.h>

typedef struct Task_ThreadStruct Task_ThreadStruct;
typedef struct Task_MemStruct Task_MemStruct;

typedef void (*Task_SignalHandler)(u64 signal);

struct Task_ThreadStruct {
	
} __attribute__ ((packed));

struct Task_MemStruct {
	u64 allocVirtMem;
	u64 allocMem;
} __attribute__ ((packed));

struct Task_TaskStruct {
	u32 cpuId, priority;
	u64 pid;
	u64 vRuntime, resRuntime;

	Task_ThreadStruct *thread;
	Task_MemStruct *mem;

	List list;
	RBNode rbNode;
 
	hal_TaskStruct hal;
} __attribute__ ((packed));

typedef struct Task_TaskStruct Task_TaskStruct;

typedef union Task_Union {
	Task_TaskStruct task;
	u8 krlStk[Task_krlStkSize];
} Task_union;

#endif