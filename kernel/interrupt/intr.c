#include <interrupt/api.h>
#include <screen/screen.h>
#include <hal/interrupt/api.h>
#include <cpu/desc.h>
#include <lib/bit.h>

cpu_definevar(u64[4], intr_msk);
cpu_definevar(u32, intr_freeCnt);
cpu_definevar(u32, intr_useCnt);
cpu_definevar(SpinLock, intr_mskLck);

void intr_initDesc(intr_Desc *desc, void (*handler)(u64), u64 param, char *name, intr_Ctrl *ctrl) {
	desc->handler = handler;
	desc->param = param;
	desc->name = name;
	desc->ctrl = ctrl;
}

static SpinLock intr_allocLck;

int intr_init() {
	SpinLock_init(&intr_allocLck);
	if (hal_intr_init() == res_FAIL) return res_FAIL;
	return res_SUCC;
}

int intr_initCpuVar(int idx) {
    int res = hal_intr_initCpuVar(idx);
    return res;
}

int intr_alloc(intr_Desc *desc, int intrNum) {
    // make it balanced
	SpinLock_lock(&intr_allocLck);
    u32 bstCpu = 0;
    for (int i = 0; i < cpu_num; i++)
        if (cpu_getCpuVar(i, intr_freeCnt) > cpu_getCpuVar(bstCpu, intr_freeCnt))
            bstCpu = i;
    if (cpu_getCpuVar(bstCpu, intr_freeCnt) < intrNum) {
		SpinLock_unlock(&intr_allocLck);
		return res_FAIL;
	}
    for (int i = 0x0; i < 0x100 - intrNum; i++) {
        int flag = -1;
        for (int j = intrNum - 1; j >= 0; j--)
            if (cpu_getCpuVar(bstCpu, intr_msk[(i + j) / 64]) & (1ull << ((i + j) % 64))) {
                flag = j;
                break;
            }
        if (flag != -1) { i += flag; continue; }
        desc->cpuId = bstCpu;
        desc->vecId = i;
        goto succ;
    }
	SpinLock_unlock(&intr_allocLck);
    return res_FAIL;
    succ:
    *cpu_cpuPtr(bstCpu, intr_freeCnt) -= intrNum;
    *cpu_cpuPtr(bstCpu, intr_useCnt) += intrNum;
    for (int i = 0; i < intrNum; i++) {
        desc[i].cpuId = bstCpu;
        desc[i].vecId = desc[0].vecId + i;
        bit_set1(cpu_cpuPtr(bstCpu, intr_msk[desc[i].vecId / 64]), desc[i].vecId % 64);
    }
	SpinLock_unlock(&intr_allocLck);
    printk(screen_log, "intr: alloc on cpu %d, vec:%d~%d\n", bstCpu, desc[0].vecId, desc[0].vecId + intrNum - 1);
    hal_intr_setInCpuDesc(desc, intrNum);
	return res_SUCC;
}

int intr_free(intr_Desc *desc, int intrNum) {
	SpinLock_lock(&intr_allocLck);
	for (int i = 0; i < intrNum; i++) {
        int idx = desc->cpuId;
		bit_set0(cpu_cpuPtr(idx, intr_msk[desc[i].vecId / 64]), desc[i].vecId % 64);
		(*cpu_cpuPtr(idx, intr_freeCnt))++;
        (*cpu_cpuPtr(idx, intr_useCnt))--;
	}
	SpinLock_unlock(&intr_allocLck);
	return res_SUCC;
}
