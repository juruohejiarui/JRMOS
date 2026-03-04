#include <softirq/api.h>
#include <screen/screen.h>
#include <interrupt/api.h>
#include <task/api.h>
#include <cpu/api.h>
#include <lib/bit.h>

cpu_definevar(softirq_Desc *[64], softirq_desc);
cpu_definevar(Atomic, softirq_flag);
cpu_definevar(u32, softirq_freeCnt);
cpu_definevar(u32, softirq_useCnt);
cpu_definevar(u64, softirq_msk);

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
    int trgCpu = -1;
    for (int i = 0; i < cpu_num; i++) {
        if (!cpu_getCpuVar(i, softirq_freeCnt)) continue;
        trgCpu = (!trgCpu || cpu_getCpuVar(i, softirq_freeCnt) < cpu_getCpuVar(trgCpu, softirq_freeCnt) ? i : trgCpu);
    }
    SpinLock_unlock(&_allocLck);
    return res_FAIL;

    Succ:
    desc->cpuId = trgCpu;
    for (int i = 0; i < 64; i++) if (!(cpu_getCpuVar(trgCpu, softirq_msk) & (1ul << i))) {
        desc->vecId = i;
        bit_set1(cpu_cpuPtr(trgCpu, softirq_msk), i);
        break;
    }
    (*cpu_cpuPtr(trgCpu, softirq_freeCnt))--;
    (*cpu_cpuPtr(trgCpu, softirq_useCnt))++;
    SpinLock_unlock(&_allocLck);
    return res_SUCC;
}

int softirq_free(softirq_Desc *desc) {
    SpinLock_lock(&_allocLck);
    register int i = desc->cpuId;
    (*cpu_cpuPtr(i, softirq_freeCnt))--;
    (*cpu_cpuPtr(i, softirq_useCnt))++;
    bit_set0(cpu_cpuPtr(i, softirq_msk), desc->vecId);
    SpinLock_unlock(&_allocLck);
    return res_SUCC;
}

void softirq_send(softirq_Desc *desc) { Atomic_bts(cpu_cpuPtr(desc->cpuId, softirq_flag), desc->vecId); }

void softirq_scan() {
    for (int i = 0; i < 64; i++) if (cpu_ptr(softirq_flag)->value & (1ul << i)) {
        softirq_Desc *desc = cpu_getvar(softirq_desc[i]);
        Atomic_btr(cpu_ptr(softirq_flag), i);
        if (desc == NULL || desc->handler == NULL) {
            printk(screen_err, "sirq: cpu #%d no handler for sirq #%d\n",
                task_cur->cpuId, i);
        } else {
            intr_mask();
            desc->handler(desc->param);
            intr_unmask();
        }
    }
}