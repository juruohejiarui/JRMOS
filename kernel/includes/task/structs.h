#ifndef __TASK_STRUCTS_H__
#define __TASK_STRUCTS_H__

#include <task/constant.h>
#include <lib/list.h>
#include <lib/rbtree.h>
#include <hal/task/structs.h>

typedef struct task_ThreadStruct task_ThreadStruct;
typedef struct task_MemStruct task_MemStruct;

typedef void (*task_SignalHandler)(u64 signal);

struct task_ThreadStruct {
	
} __attribute__ ((packed));

struct task_MemStruct {
	u64 allocVirtMem;
	u64 allocMem;
} __attribute__ ((packed));

struct task_TaskStruct {
	u32 cpuId, priority;
	u64 pid;
	u64 vRuntime, resRuntime;

	task_ThreadStruct *thread;
	task_MemStruct *mem;

	List list;
	RBNode rbNode;
 
	hal_task_TaskStruct hal;
} __attribute__ ((packed));

typedef struct task_TaskStruct task_TaskStruct;

typedef union task_Union {
	task_TaskStruct task;
	u8 krlStk[task_krlStkSize];
} task_union;

#endif