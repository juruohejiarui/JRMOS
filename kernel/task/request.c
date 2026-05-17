#include "inner.h"
#include <task/request.h>
#include <screen/screen.h>

__always_inline__ void _waitReq() {
	Atomic_inc(&task_cur->reqWait);
}

__always_inline__ void _finishReq(task_Thread *thd) {
	Atomic_dec(&thd->reqWait);
	if (thd->reqWait.value == 0)
		task_sche_wake(thd);
}

__optimize__ void task_Request_init(task_Request *req, u64 flags) {
	req->src = task_cur;
	req->flags = flags;
	req->stage.value = task_Request_stage_Wait;
	hal_hw_mfence();
}

__optimize__ void task_Request_send(task_Request *req) {
	// if (req->stage.value == task_Request_stage_Finish) return ;
	if (req->flags & task_Request_flags_Abort)
		_waitReq();
	else while (__unlikely____(req->stage.value != task_Request_stage_Finish)) 
		task_sche_yield();
}

__optimize__ void task_Request_response(task_Request *req) {
	Atomic_inc(&req->stage);
	if (req->flags & task_Request_flags_Abort) 
		_finishReq(req->src);
}