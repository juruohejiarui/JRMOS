#include <timer/api.h>
#include <task/api.h>
#include <hal/timer/api.h>

Atomic timer_jiff;

void timer_updSirq() {
	Atomic_inc(&timer_jiff);
}

void timer_mdelay(u64 msec) {
	u64 jiff = timer_getJiff();
	while (timer_getJiff() - jiff < msec) {
		task_sche_yield();
	}
}

void timer_initRq(timer_Request *req, u64 jiff, void (*handler)(u64), u64 param) {
	req->handler = handler;
	req->param = param;
	req->jiff = timer_jiff.value + jiff;
}

void timer_insRq(timer_Request *req) {
}

int timer_init() {
	return res_SUCC;
}