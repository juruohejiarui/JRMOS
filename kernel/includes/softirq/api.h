#ifndef __SOFTIRQ_API_H__
#define __SOFTIRQ_API_H__

#include <softirq/desc.h>

void softirq_init(softirq_Desc *desc, void (*handler)(u64), u64 param);

int softirq_alloc(softirq_Desc *desc);

int softirq_free(softirq_Desc *desc);
#endif