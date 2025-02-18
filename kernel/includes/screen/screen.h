#ifndef __SCREEN_SCREEN_H__
#define __SCREEN_SCREEN_H__

#include <lib/dtypes.h>

#define screen_charWidth	8
#define screen_charHeight	16

typedef struct screen_Info {
	u32 horRes;
	u32 verRes;
	u32 pixelPreLine;
	
	// physical address of frame buffer
	u64 frameBufBase;
	u64 frameBufSize;
} screen_Info;

extern screen_Info *screen_info;
void screen_init();

#endif