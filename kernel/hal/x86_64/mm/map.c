#include <hal/mm/map.h>

static __always_inline__ u64 _cvtAttr() {

}
int hal_mm_map(u64 virt, u64 phys, u64 attr, u64 size) {
    attr = _cvtAttr(attr);
}