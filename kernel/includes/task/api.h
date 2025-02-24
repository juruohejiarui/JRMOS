#ifndef __TASK_API_H__
#define __TASK_API_H__

#include <hal/task/api.h>

#ifdef HAL_TASK_GETLEVEL
#define task_getLevel hal_task_getLevel
#else
#error No definition of task_getLevel() for this arch
#endif
#endif