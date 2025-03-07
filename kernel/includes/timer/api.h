#ifndef __TIMER_API_H__
#define __TIMER_API_H__

#include <timer/desc.h>

void timer_initRq(timer_Request *req, u64 jiff, void (*handler)(u64), u64 param);

void timer_insRq(timer_Request *req);

int timer_init();

// check and update the flag of soft interrupt
void timer_updSirq();

#endif