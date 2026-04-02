#include <hardware/datetime.h>
#include <hal/hardware/datetime.h>

void hw_datetime_now(Datetime *result) {
	hal_hw_datetime_now(result);
}