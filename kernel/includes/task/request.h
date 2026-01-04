#ifndef __TASK_REQUEST_H__
#define __TASK_REQUEST_H__

#include <lib/dtypes.h>
#include <task/api.h>

typedef struct task_Request {
	task_TaskStruct *src;
	u64 flags;
	Atomic stage;
} task_Request;

#define task_Request_flags_Abort 	0x1ul

#define task_Request_stage_Wait 	0
#define task_Request_stage_Finish	1

void task_Request_init(task_Request *req, u64 flags);

void task_Request_send(task_Request *req);

void task_Request_response(task_Request *req);

__always_inline__ int task_Request_isFinished(task_Request *req) {
	return req->stage.value == task_Request_stage_Finish;
}

// give up all the request, must guarantee that those request will not be responsed
__always_inline__ void task_Request_giveUp() {
	task_current->reqWait.value = 0;
}
#endif 