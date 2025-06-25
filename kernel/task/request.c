#include <task/request.h>

void task_Request_send(task_Request *req) {
	Atomic_btr(&req->flags, 1);
	if (req->flags.value & task_Request_Flag_Abort) task_sche_waitReq();
}