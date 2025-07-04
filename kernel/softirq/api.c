#include <softirq/api.h>
#include <screen/screen.h>
#include <interrupt/api.h>
#include <task/api.h>
#include <cpu/api.h>
#include <lib/bit.h>

static SpinLock _allocLck;
int softirq_init() {
    SpinLock_init(&_allocLck);
    return res_SUCC;
}

void softirq_initDesc(softirq_Desc *desc, void (*handler)(u64), u64 param) {
    desc->handler = handler;
    desc->param = param;
    desc->cpuId = cpu_num;
    desc->vecId = 64;
}

int softirq_alloc(softirq_Desc *desc) {
    SpinLock_lock(&_allocLck);
    cpu_Desc *trgCpu = NULL;
    for (int i = 0; i < cpu_num; i++) {
        cpu_Desc *cpu = &cpu_desc[i];
        if (!cpu->sirqFree) continue;
        trgCpu = (trgCpu == NULL || trgCpu->sirqFree < cpu->sirqFree ? cpu : trgCpu);
    }
    SpinLock_unlock(&_allocLck);
    return res_FAIL;

    Succ:
    desc->cpuId = trgCpu - cpu_desc;
    for (int i = 0; i < 64; i++) if (!(trgCpu->sirqMsk & (1ul << i))) {
        desc->vecId = i;
        bit_set1(&trgCpu->sirqMsk, i);
        break;
    }
    trgCpu->sirqUsage++, trgCpu->sirqFree--;
    SpinLock_unlock(&_allocLck);
    return res_SUCC;
}

int softirq_free(softirq_Desc *desc) {
    SpinLock_lock(&_allocLck);
    register cpu_Desc *cpu = &cpu_desc[desc->cpuId];
    cpu->sirqUsage--, cpu->sirqFree++;
    bit_set0(&cpu->sirqMsk, desc->vecId);
    SpinLock_unlock(&_allocLck);
    return res_SUCC;
}

void softirq_send_self(u64 irq) { Atomic_bts(&cpu_desc[task_current->cpuId].sirqFlag, irq); }
void softirq_send_all(u64 irq) {
    for (int i = 0; i < cpu_num; i++) softirq_send(i, irq);
}
void softirq_send_allExcludSelf(u64 irq) {
    for (int i = 0; i < cpu_num; i++) if (i != task_current->cpuId) softirq_send(i, irq);
}
void softirq_send(u32 cpuId, u64 irq) { Atomic_bts(&cpu_desc[cpuId].sirqFlag, irq); }

void softirq_scan() {
    cpu_Desc *curCpu = &cpu_desc[task_current->cpuId];
    for (int i = 0; i < 64; i++) if (cpu_getvar(sirqFlag.value) & (1ul << i)) {
        softirq_Desc *desc = cpu_getvar(sirq[i]);
        Atomic_btr(&curCpu->sirqFlag, i);
        if (desc == NULL || desc->handler == NULL) {
            printk(screen_err, "sirq: cpu #%d no handler for sirq #%d\n",
                task_current->cpuId, i);
        } else {
            intr_mask();
            desc->handler(desc->param);
            intr_unmask();
        }
    }
}