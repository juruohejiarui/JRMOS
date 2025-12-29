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

typedef void (*task_SignalHandler)(u64 signal);

typedef struct task_MemStruct {
	Atomic krlTblModiJiff;

	SpinLock allocLck;
	u64 allocVirtMem;
	u64 allocMem;

	RBTree slabRecord;
	mm_MapInfo mapInfo;
	
	SpinLock pgTblLck;
	hal_task_MemStruct hal;
} task_MemStruct;

struct task_ThreadStruct {
	// kernel page table version of this thread.
	task_MemStruct mem;

	task_SignalHandler sigHandler[task_nrSignal];
	u64 sigParam[task_nrSignal];

	SafeList tskList;

	SafeList openFileLst;
	SafeList openDirLst;

	hal_task_ThreadStruct hal;
} __attribute__ ((packed));

struct task_TaskStruct {
	u32 cpuId, priority;
	u64 flag;
	u64 pid;
	volatile u64 state;
	i64 vRuntime;
	// the counter of io requests / collaboration requests.
	// Move to sleep state when this value is greater than 0.
	// Move back to running state / idle state when this value is reduced to 0.
	Atomic reqWait;

	task_ThreadStruct *thread;

	RBNode rbNd;

	ListNode threadNd;
	
	ListNode scheNd;


	Atomic signal;
 
	hal_task_TaskStruct hal;
} __attribute__ ((packed));

typedef struct task_TaskStruct task_TaskStruct;

typedef union task_Union {
	task_TaskStruct task;
	u8 krlStk[task_krlStkSize];
} task_Union;

typedef struct task_MgrStruct {
	RBTree tasks[cpu_mxNum];
	ListNode preemptTsks[cpu_mxNum];
	SpinLock scheLck[cpu_mxNum];
	Atomic scheMsk[cpu_mxNum];
	SafeList freeTsks, sleepTsks;
} task_MgrStruct;

#endif