#ifndef __TIMER_DESC_H__
#define __TIMER_DESC_H__

#include <lib/atomic.h>

// timer min tick (in nano second)
#define timer_minTick   (1000000ul)

Atomic timer_jiff;
#endif