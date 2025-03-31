#ifndef __SOFTIRQ_API_H__
#define __SOFTIRQ_API_H__

#include <softirq/desc.h>

void softirq_init(softirq_Desc *desc, void (*handler)(u64), u64 param);

int softirq_alloc(softirq_Desc *desc);

int softirq_free(softirq_Desc *desc);

void softirq_send_self(u64 irq);
void softirq_send_all(u64 irq);
void softirq_send_allExcludSelf(u64 irq);
void softirq_send(u32 cpuId, u64 irq);

void softirq_scan();
#endif