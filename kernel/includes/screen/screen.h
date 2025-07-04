#ifndef __SCREEN_SCREEN_H__
#define __SCREEN_SCREEN_H__

#include <lib/dtypes.h>
#include <stdarg.h>

#define screen_charWidth	8
#define screen_charHeight	16

#define RED		0xff0000
#define GREEN	0x00ff00
#define BLUE	0x1010ff
#define BLACK	0x000000
#define YELLOW	0xffff00
#define ORANGE	0xffa000
#define WHITE	0xffffff

#define screen_color(fcol, bcol) ((u64)(fcol) | ((u64)(bcol) << 32))
#define screen_log 		screen_color(WHITE, 	BLACK)
#define screen_err 		screen_color(RED, 		BLACK)
#define screen_warn 	screen_color(ORANGE, 	BLACK)
#define screen_succ 	screen_color(GREEN, 	BLACK)


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

int screen_enableBuf();

void printk(u64 col, const char *fmt, ...);

void printu(u64 col, const char *fmt, ...);

#endif