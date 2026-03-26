#include <task/request.h>
#include <screen/screen.h>

__optimize__ void task_Request_init(task_Request *req, u64 flags) {
	req->src = task_cur;
	req->flags = flags;
	req->stage.value = task_Request_stage_Wait;
	hal_hw_mfence();
}

__optimize__ void task_Request_send(task_Request *req) {
	// if (req->stage.value == task_Request_stage_Finish) return ;
	if (req->flags & task_Request_flags_Abort)
		task_sche_waitReq();
}

__optimize__ void task_Request_response(task_Request *req) {
	Atomic_inc(&req->stage);
	if (req->flags & task_Request_flags_Abort) task_sche_finishReq(req->src);
}