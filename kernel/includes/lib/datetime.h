#ifndef __LIB_DATETIME_H__
#define __LIB_DATETIME_H__

#include <lib/dtypes.h>
#include <lib/algorithm.h>

typedef struct Date {
	u16 year;
	u8 month;
	u8 day;
} Date;

typedef struct Timestamp {
	u8 hour;
	u8 minute;
	u8 second;
	u16 millsec;
} Timestamp;

typedef struct Datetime {
	Date date;
	Timestamp time;
} Datetime;

__always_inline__ int date_cmp(Date *a, Date *b) {
	register int t = cmp(a->year, b->year);
	if (t) return t;
	t = cmp(a->month, b->month);
	if (t) return t;
	t = cmp(a->day, b->day);
	return t;
}

__always_inline__ int time_cmp(Timestamp *a, Timestamp *b) {
	register int t = cmp(a->hour, b->hour);
	if (t) return t;
	t = cmp(a->minute, b->minute);
	if (t) return t;
	t = cmp(a->second, b->second);
	if (t) return t;
	t = cmp(a->millsec, b->millsec);
	return t;
}

__always_inline__ int datetime_cmp(Datetime *a, Datetime *b) {
	register int t = date_cmp(&a->date, &b->date);
	if (t) return t;
	t = time_cmp(&a->time, &b->time);
	return t;
}
#endif