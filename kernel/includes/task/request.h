#ifndef __TASK_REQUEST_H__
#define __TASK_REQUEST_H__

#include <lib/dtypes.h>
#include <task/api.h>

typedef struct task_Request {
	task_TaskStruct *src;
	Atomic flags;
} task_Request;

#define hw_Request_Flag_Abort 	0x1ul
#define hw_Request_Flag_Finish 	0x2ul

static __always_inline__ void task_Request_init(task_Request *req, u64 flags) {
	req->src = task_current;
	req->flags.value = flags;
}

static __always_inline__ void task_Request_send(task_Request *req) {
	Atomic_btr(&req->flags, 1);
	if (req->flags.value & hw_Request_Flag_Abort) task_sche_waitReq();
}

static __always_inline__ void task_Request_response(task_Request *req) {
	Atomic_bts(&req->flags, 1);
	if (req->flags.value & hw_Request_Flag_Abort) task_sche_finishReq(req->src);
}

static __always_inline__ int task_Request_isFinished(task_Request *req) {
	return req->flags.value & hw_Request_Flag_Finish;
}

#endif 