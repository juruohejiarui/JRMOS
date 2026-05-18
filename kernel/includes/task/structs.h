#ifndef __TASK_STRUCTS_H__
#define __TASK_STRUCTS_H__

#include <task/constant.h>
#include <task/semaphore.h>
#include <lib/atomic.h>
#include <lib/rbtree.h>
#include <hal/task/structs.h>
#include <cpu/desc.h>
#include <mm/desc.h>
#include <mm/map.h>

typedef struct task_Process task_Process;
typedef struct task_Thread task_Thread;

typedef void (*task_SignalHandler)(i64 signal);

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

struct task_Process {
	// kernel page table version of this proc.
	task_MemStruct mem;

	u64 state;
	u64 id;

	task_Process *parent;

	task_SignalHandler sigHandler[task_nrSignal];
	u64 sigParam[task_nrSignal];

	SafeList thdLst, children;

	ListNode childNd;

	// opened file list
	SafeList fileLst;
	// opened directory list
	SafeList dirLst;

	task_Thread *mainThd;

	task_Semaphore waitSem;

	hal_task_Process hal;
} __attribute__ ((packed));

struct task_Thread {
	u32 cpuId, priority;
	u64 flag;
	u64 pid;
	volatile u64 state;
	i64 vRuntime;
	// the counter of io requests / collaboration requests.
	// Move to sleep state when this value is greater than 0.
	// Move back to running state / idle state when this value is reduced to 0.
	Atomic reqWait;

	task_Process *proc;

	RBNode rbNd;

	ListNode thdNd, scheNd, semNd;

	Atomic signal;
	i64 workingSignal;
 
	hal_task_Thread hal;
} __attribute__ ((packed));

typedef union task_Union {
	task_Thread thd;
	u8 krlStk[task_krlStkSize];
} task_Union;

#endif