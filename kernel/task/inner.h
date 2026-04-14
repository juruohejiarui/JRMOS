#include <task/api.h>
#include <task/request.h>

extern Atomic task_pidCnt, task_threadCnt;

extern SafeList task_freeTsks, task_sleepTsks;

void task_sche_init();