#include <hal/hardware/gpu/api.h>
#include <hal/hardware/gpu/intel.h>

int hal_hw_gpu_init() {
	int res = (hal_hw_gpu_intel_init() == res_SUCC);
	return res ? res_SUCC : res_FAIL; 
}