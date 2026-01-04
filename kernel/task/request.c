#include <task/request.h>
#include <screen/screen.h>

void task_Request_init(task_Request *req, u64 flags) {
	req->src = task_current;
	req->flags = flags;
	req->stage.value = task_Request_stage_Wait;
	hal_hw_mfence();
}

void task_Request_send(task_Request *req) {
	// if (req->stage.value == task_Request_stage_Finish) return ;
	if (req->flags & task_Request_flags_Abort)
		task_sche_waitReq();
}

void task_Request_response(task_Request *req) {
	Atomic_inc(&req->stage);
	if (req->flags & task_Request_flags_Abort) task_sche_finishReq(req->src);
}