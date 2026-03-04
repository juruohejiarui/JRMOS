#include <cpu/desc.h>
#include <cpu/api.h>
#include <lib/string.h>
#include <mm/mm.h>
#include <screen/screen.h>

u32 cpu_num;
u32 cpu_bspIdx;
void **cpu_bsAddr;

cpu_definevar(u64, cpu_bsAddr);
cpu_definevar(u64, cpu_id);
cpu_definevar(u64, cpu_state);
cpu_definevar(u64, cpu_flag);

// this function should be called after setting cpu_num
int cpu_initVar() {
    u64 sz = &cpu_symbol_epercpu - &cpu_symbol_percpu;
    cpu_bsAddr = mm_kmalloc(sizeof(void *) * cpu_num, mm_Attr_Shared, NULL);

    if (cpu_bsAddr == NULL) {
        printk(screen_err, "cpu: failed to alloc cpu_bsAddr.\n");
        return res_FAIL;
    }
    for (int i = 0; i < cpu_num; i++) {
        cpu_bsAddr[i] = mm_kmalloc(sz, mm_Attr_Shared, NULL);
        memset(cpu_bsAddr[i], 0, sz);

        printk(screen_log, "cpu: bsAddr[%d]: %p\n", i, cpu_bsAddr[i]);
    }
    return sz;
}