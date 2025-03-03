#include <interrupt/api.h>
#include <hal/interrupt/interrupt.h>

int intr_init() {
	if (hal_intr_init() == res_FAIL) return res_FAIL;
	return res_SUCC;
}