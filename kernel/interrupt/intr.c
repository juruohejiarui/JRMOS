#include <interrupt/api.h>
#include <hal/interrupt/api.h>
#include <cpu/desc.h>
#include <lib/bit.h>

void intr_initDesc(intr_Desc *desc, void (*handler)(u64), u64 param, char *name, intr_Ctrl *ctrl) {
	desc->handler = handler;
	desc->param = param;
	desc->name = name;
	desc->ctrl = ctrl;
}

static SpinLock intr_allocLck;

int intr_init() {
	SpinLock_lock(&intr_allocLck);
	if (hal_intr_init() == res_FAIL) return res_FAIL;
	return res_SUCC;
}

int intr_alloc(intr_Desc *desc, int intrNum) {
    // make it balanced
	SpinLock_lock(&intr_allocLck);
    u32 bstCpu = 0;
    for (int i = 0; i < cpu_num; i++)
        if (cpu_desc[i].intrFree > cpu_desc[bstCpu].intrFree)
            bstCpu = i;
    if (cpu_desc[bstCpu].intrFree < intrNum) {
		SpinLock_unlock(&intr_allocLck);
		return res_FAIL;
	}
    for (int i = 0x0; i < 0x100 - intrNum; i++) {
        int flag = -1;
        for (int j = intrNum - 1; j >= 0; j--)
            if (cpu_desc[bstCpu].intrMsk[(i + j) / 64] & (1ull<< ((i + j) % 64))) {
                flag = j;
                break;
            }
        if (flag != -1) i += flag;
        desc->cpuId = bstCpu;
        desc->vecId = i;
        goto succ;
    }
	SpinLock_unlock(&intr_allocLck);
    return res_FAIL;
    succ:
    cpu_desc[bstCpu].intrFree -= intrNum;
    cpu_desc[bstCpu].intrUsage += intrNum;
    for (int i = 0; i < intrNum; i++) {
        desc[i].cpuId = bstCpu;
        desc[i].vecId = desc[0].vecId + i;
        bit_set1(&cpu_desc[bstCpu].intrMsk[desc[i].vecId / 64], desc[i].vecId % 64);
    }
	SpinLock_unlock(&intr_allocLck);
	return res_SUCC;
}

int intr_free(intr_Desc *desc, int intrNum) {
	SpinLock_lock(&intr_allocLck);
	for (int i = 0; i < intrNum; i++) {
		cpu_Desc *cpu = &cpu_desc[desc[i].cpuId];
		bit_set0(&cpu->intrMsk[desc[i].vecId / 64], desc[i].vecId % 64);
		cpu->intrFree++;
		cpu->intrUsage--;
	}
	SpinLock_unlock(&intr_allocLck);
	return res_SUCC;
}
