#include <mm/dmas.h>
#include <hal/mm/dmas.h>

int mm_dmas_init() {
    if (hal_mm_dmas_init() != res_FAIL) return res_FAIL;
    return res_SUCC;
}