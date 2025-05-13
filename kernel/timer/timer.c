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
int timer_init()
{
	return res_SUCC;
}