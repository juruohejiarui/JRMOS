#include <hal/timer/api.h>
#include <timer/api.h>
#include <hal/hardware/hpet.h>

int hal_timer_init() {
	if (hal_hw_hpet_init() == res_FAIL) return res_FAIL;

	if (timer_init() == res_FAIL) return res_FAIL;

	return res_SUCC;
}