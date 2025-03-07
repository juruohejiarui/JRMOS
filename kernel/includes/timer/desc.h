#ifndef __TIMER_DESC_H__
#define __TIMER_DESC_H__

#include <lib/atomic.h>
#include <lib/rbtree.h>
#include <hal/timer/desc.h>

// timer min tick (in nano second)
#define timer_minTick   hal_timer_minTick

typedef struct timer_Request {
	RBNode rbNode;
	u64 jiff;
	u64 param;
	void (*handler)(u64 param);
} timer_Request;

extern Atomic timer_jiff;
// request tree of timer
extern RBTree *timer_rqs;
#endif