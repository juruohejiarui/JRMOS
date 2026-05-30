#include "inner.h"
#include <hardware/datetime.h>
#include <lib/string.h>
#include <screen/screen.h>
#include <cpu/api.h>

cpu_definevar(u64, _preSwitchCnt);
Datetime lstDt;

void task_init() {
	_sche_init();
	_mgr_init();
	_signal_init();
	hw_datetime_now(&lstDt);
	lstDt.time.millsec = 0;
}

void task_log() {
	Datetime dt;
	hw_datetime_now(&dt);
	dt.time.millsec = 0;
	if (datetime_cmp(&dt, &lstDt) <= 0) return;
	int outlen = 0;
	char out[512], perCpu[32];
	static const char perCpuFmt[] = "#%02d: %4d ";
	for (int i = 0; i < cpu_num; i++) {
		int len = sprintf(perCpu, perCpuFmt, i, cpu_getCpuVar(i, task_sche_switchCnt) - cpu_getCpuVar(i, _preSwitchCnt));
		cpu_setCpuVar(i, _preSwitchCnt, cpu_getCpuVar(i, task_sche_switchCnt));
		// append perCpu to out
		memcpy(perCpu, out + outlen, len);
		outlen += len;
	}
	memcpy(&dt, &lstDt, sizeof(Datetime));
	out[outlen] = '\0';
	printk(screen_log, "task: log: %2d:%2d:%2d: %s\r", dt.time.hour, dt.time.minute, dt.time.second, out);
}