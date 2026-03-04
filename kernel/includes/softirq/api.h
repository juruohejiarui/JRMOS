#ifndef __SOFTIRQ_API_H__
#define __SOFTIRQ_API_H__

#include <softirq/desc.h>

int softirq_init();

void softirq_initDesc(softirq_Desc *desc, void (*handler)(u64), u64 param);

int softirq_alloc(softirq_Desc *desc);

int softirq_free(softirq_Desc *desc);

void softirq_send(softirq_Desc *desc);

void softirq_scan();
#endif