#ifndef __TIMER_API_H__
#define __TIMER_API_H__

#include <timer/desc.h>
#include <hal/timer/api.h>

void timer_initRq(timer_Request *req, u64 jiff, void (*handler)(u64), u64 param);

void timer_insRq(timer_Request *req);

int timer_init();

// check and update the flag of soft interrupt
void timer_updSirq();

__always_inline__ u64 timer_getJiff() { return timer_jiff.value; }

void timer_mdelay(u64 msec);

#endif