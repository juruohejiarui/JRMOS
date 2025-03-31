#include <softirq/api.h>
#include <screen/screen.h>
#include <interrupt/api.h>
#include <task/api.h>
#include <cpu/api.h>

void softirq_init(softirq_Desc *desc, void (*handler)(u64), u64 param) {
    desc->handler = handler;
    desc->param = param;
    desc->cpuId = cpu_num;
    desc->vecId = 64;
}

int softirq_alloc(softirq_Desc *desc) {

}

int softirq_free(softirq_Desc *desc) {

}

void softirq_send_self(u64 irq) { Atomic_bts(&cpu_desc[task_current->cpuId].sirqFlag, irq); }

void softirq_send(u32 cpuId, u64 irq) { Atomic_bts(&cpu_desc[cpuId].sirqFlag, irq); }

void softirq_scan() {
    cpu_Desc *curCpu = &cpu_desc[task_current->cpuId];
    for (int i = 0; i < 64; i++) if (cpu_getvar(sirqFlag.value) & (1ul << i)) {
        softirq_Desc *desc = cpu_getvar(sirq[i]);
        Atomic_btc(&curCpu->sirqFlag, i);
        if (desc == NULL || desc->handler == NULL) {
            printk(RED, BLACK, "sirq: cpu #%d no handler for sirq #%d\n",
                task_current->cpuId, i);
        } else {
            intr_mask();
            desc->handler(desc->param);
            intr_unmask();
        }
    }
}