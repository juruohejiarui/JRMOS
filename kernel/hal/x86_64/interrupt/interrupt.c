#include <hal/interrupt/interrupt.h>
#include <hal/hardware/apic.h>


int hal_intr_allocVec() {
    
}

int hal_intr_init() {
    if (hal_hw_apic_init() == res_FAIL) return res_FAIL;
    return res_SUCC;
}