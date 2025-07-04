#ifndef __TASK_REQUEST_H__
#define __TASK_REQUEST_H__

#include <lib/dtypes.h>
#include <task/api.h>

typedef struct task_Request {
	task_TaskStruct *src;
	Atomic flags;
} task_Request;

#define task_Request_Flag_Abort 	0x1ul
#define task_Request_Flag_Finish 	0x2ul

__always_inline__ void task_Request_init(task_Request *req, u64 flags) {
	req->src = task_current;
	req->flags.value = flags;
}

void task_Request_send(task_Request *req);

__always_inline__ void task_Request_response(task_Request *req) {
	Atomic_bts(&req->flags, 1);
	if (req->flags.value & task_Request_Flag_Abort) task_sche_finishReq(req->src);
}

__always_inline__ int task_Request_isFinished(task_Request *req) {
	return req->flags.value & task_Request_Flag_Finish;
}

// give up all the request, must guarantee that those request will not be responsed
__always_inline__ void task_Request_giveUp() {
	task_current->reqWait.value = 0;
}
#endif 