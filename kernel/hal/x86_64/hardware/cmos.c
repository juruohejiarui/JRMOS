#include <hal/hardware/datetime.h>
#include <hal/hardware/reg.h>
#include <hal/hardware/io.h>

#define cmos_Address	0x70
#define cmos_Data		0x71

__always_inline__ u8 _getRtcReg(int reg) {
	hal_hw_io_out8(cmos_Address, reg);
	return hal_hw_io_in8(cmos_Data);
}

__always_inline__ int _getInProgFlag() { return _getRtcReg(0x0a) & 0x80; }

__optimize__ void hal_hw_datetime_now(Datetime *result) {
	result->time.millsec = 0;
	u8 sec, min, hr, day, month; u16 year;
	u8 lst_sec, lst_min, lst_hr, lst_day, lst_month; u16 lst_year;

	while (_getInProgFlag()) ;
	sec = _getRtcReg(0x00);
	min = _getRtcReg(0x02);
	hr = _getRtcReg(0x04);
	day = _getRtcReg(0x07);
	month = _getRtcReg(0x08);
	year = _getRtcReg(0x09);

	do {
		lst_sec = sec;
		lst_min = min;
		lst_hr = hr;
		lst_day = day;
		lst_month = month;
		lst_year = year;
		while (_getInProgFlag()) ;
		sec = _getRtcReg(0x00);
		min = _getRtcReg(0x02);
		hr = _getRtcReg(0x04);
		day = _getRtcReg(0x07);
		month = _getRtcReg(0x08);
		year = _getRtcReg(0x09);
	} while (__unlikely__(lst_year != year) || __unlikely__(lst_month != month) || __unlikely__(lst_day != day)
			|| __unlikely__(lst_hr != hr) || __unlikely__(lst_min != min) || __unlikely__(lst_sec != sec));
	
	u8 regB = _getRtcReg(0x0b);
	
	if (!(regB & 0x04)) {
		sec = (sec & 0x0f) + ((sec / 16) * 10);
		min = (min & 0x0f) + ((min / 16) * 10);
		hr = ((hr & 0x0f) + (((hr & 0x70) / 16) * 10)) | (hr & 0x80);
		day = (day & 0x0f) + ((day / 16) * 10);
		month = (month & 0x0f) + ((month / 16) * 10);
		year = (year & 0x0f) + ((year / 16) * 10);
	}
	if (!(regB & 0x02) && (hr & 0x80))
		hr = ((hr & 0x7f) + 12) % 24;
	
	result->date.year = year;
	result->date.month = month;
	result->date.day = day;
	result->time.hour = hr;
	result->time.minute = min;
	result->time.second = sec;
}