#ifndef __TASK_STRUCTS_H__
#define __TASK_STRUCTS_H__

#include <task/constant.h>
#include <lib/atomic.h>
#include <lib/list.h>
#include <lib/rbtree.h>
#include <hal/task/structs.h>
#include <cpu/desc.h>
#include <mm/desc.h>
#include <mm/map.h>

typedef struct task_ThreadStruct task_ThreadStruct;
typedef struct task_MemStruct task_MemStruct;

typedef void (*task_SignalHandler)(u64 signal);

struct task_ThreadStruct {

	// kernel page table version of this thread.
	Atomic krlTblModiJiff;

	// memory usage
	Atomic allocVirtMem;
	Atomic allocMem;

	SpinLock pageRecordLck;
	List pgRecord;
	RBTree slabRecord;

	SpinLock pgTblLck;

	task_SignalHandler sigHandler[task_nrSignal];

	List tskList;
	SpinLock tskListLck;

	hal_task_ThreadStruct hal;
} __attribute__ ((packed));

struct task_TaskStruct {
	u32 cpuId, priority;
	u64 flag;
	u64 pid;
	u64 state;
	i64 vRuntime, resRuntime;

	task_ThreadStruct *thread;

	RBNode rbNode;
	List list;

	u64 signal;
 
	hal_task_TaskStruct hal;
} __attribute__ ((packed));

typedef struct task_TaskStruct task_TaskStruct;

typedef union task_Union {
	task_TaskStruct task;
	u8 krlStk[task_krlStkSize];
} task_Union;

typedef struct task_MgrStruct {
	RBTree tasks[cpu_mxNum];
	RBTree freeTasks;
} task_MgrStruct;

#endif