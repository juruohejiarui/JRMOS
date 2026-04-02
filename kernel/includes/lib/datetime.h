#ifndef __LIB_DATETIME_H__
#define __LIB_DATETIME_H__

#include <lib/dtypes.h>

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

#endif