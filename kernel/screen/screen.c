#include <screen/screen.h>
#include <lib/string.h>
#include <lib/spinlock.h>
#include <lib/algorithm.h>
#include <mm/mm.h>
#include <mm/dmas.h>
#include <mm/buddy.h>
#include <task/api.h>
#include <interrupt/api.h>
#include <task/syscall.h>
#include "font.h"

struct Position {
	int xRes, yRes, xPos, yPos;
	u32 *fbAddr;
	u32 fbLen;
} position;

static u32 *_bufAddr;
static u32 _lineLen[4096];
static u32 _lineSize;

#define _displayWidth (1024 * 4)
#define _displayHeight (768 * 4)

static SpinLock _printLck, _bufLck;

screen_Info *screen_info;

static void _printStr(u64 col, const char *str, int len);

void screen_init() {
	_bufAddr = NULL;
	memset(_lineLen, 0, sizeof(_lineLen));
	SpinLock_init(&_printLck);
	SpinLock_init(&_bufLck);
	
	position.xRes = screen_info->horRes & 0xffff;
	position.yRes = screen_info->verRes & 0xffff;

	position.xPos = position.yPos = 0;

	position.fbAddr = mm_dmas_phys2Virt(screen_info->frameBufBase);

	_lineSize = screen_info->pixelPreLine * screen_charHeight;

    // register syscall
    task_syscall_tbl[task_syscall_print] = _printStr;
}

int screen_enableBuf() {
    u64 bufSize = screen_info->pixelPreLine * screen_info->verRes * sizeof(u32);
    u64 log2BufSize = 0;
    while ((1ull<< log2BufSize) < bufSize) log2BufSize++;
    mm_Page *pages = mm_allocPages(max(0, log2BufSize - mm_pageShift), mm_Attr_Shared);
    if (pages == NULL) return res_FAIL;
    _bufAddr = mm_dmas_phys2Virt(mm_getPhyAddr(pages));
    printk(screen_log, "screen: buf addr=%p\ncopying...", _bufAddr);
    // copy the screen to buf
    memcpy(position.fbAddr, _bufAddr, bufSize);
    printk(screen_succ, "done\n");
}

static __optimize__ void _scroll(void) {
    int x, y;
    unsigned int *addr = position.fbAddr, 
                *addr2 = position.fbAddr + screen_charHeight * screen_info->pixelPreLine,
                *bufAddr = _bufAddr,
                *bufAddr2 = _bufAddr + screen_charHeight * screen_info->pixelPreLine;
    u64 offPerLine = screen_charHeight * screen_info->pixelPreLine;
    for (int i = 0; i < position.yPos - 1; i++) {
        u32 size = max(_lineLen[i + 1], _lineLen[i]) * screen_charWidth * sizeof(u32);
        for (int j = 0, off = 0; j < screen_charHeight; j++, off += screen_info->pixelPreLine) {
            if (_bufAddr == NULL) {
                memcpy(addr2 + off, addr + off, size);
            } else {
                memcpy(bufAddr2 + off, addr + off, size);
                memcpy(bufAddr2 + off, bufAddr + off, size);
            }
        }
        
        addr += offPerLine;       addr2 += offPerLine;
        bufAddr += offPerLine;    bufAddr2 += offPerLine;
        _lineLen[i] = _lineLen[i + 1];
    }
    memset(addr, 0, screen_info->pixelPreLine * screen_charHeight * sizeof(u32));
    if (_bufAddr != NULL) memset(bufAddr, 0, screen_info->pixelPreLine * screen_charHeight * sizeof(u32));
    _lineLen[position.yPos - 1] = 0;
}

static __optimize__ void _drawchar(unsigned int fcol, unsigned int bcol, int px, int py, char ch) {
    int x, y;
    int testVal; u64 off;
    unsigned int *addr, *bufAddr;
    unsigned char *fontp = font_ascii[ch];
        for (y = 0; y < screen_charHeight; y++, fontp++) {
            off = screen_info->pixelPreLine * (py + y) + px;
            addr = position.fbAddr + off;
            bufAddr = _bufAddr + off;
            testVal = 0x80;
            for (x = 0; x < screen_charWidth; x++, addr++, bufAddr++, testVal >>= 1) {
                *addr = ((*fontp & testVal) ? fcol : bcol);
                if (_bufAddr != NULL)
                    *bufAddr = ((*fontp & testVal) ? fcol : bcol);
            }
        }
}

__optimize__ void putchar(u64 col, char ch) {
    int i;
    if (ch == '\n') {
        position.yPos++, position.xPos = 0;
        if (position.yPos >= min(_displayHeight / screen_charHeight, position.yRes / screen_charHeight)) {
            _scroll();
			position.yPos--;
        }
    } else if (ch == '\r') {
        position.xPos = 0;
    } else if (ch == '\b') {
        if (position.xPos) position.xPos--;
        else position.yPos--, position.xPos = position.xRes / screen_charWidth;
    } else if (ch == '\t') {
        do {
            putchar(col, ' ');
        } while (position.xPos & 3);
    } else {
        if (position.xPos == min(_displayWidth / screen_charWidth, position.xRes / screen_charWidth))
            putchar(col, '\n');
        _drawchar(col & 0xffffffff, (col >> 32) & 0xffffffff, screen_charWidth * position.xPos, screen_charHeight * position.yPos, ch);
        position.xPos++;
        _lineLen[position.yPos] = max(_lineLen[position.yPos], position.xPos);
    }
}

static __optimize__ void _printStr(u64 col, const char *str, int len) {
    // close the interrupt if it is open now
	u64 prevState = intr_state();
	intr_mask();
	SpinLock_lock(&_printLck);
    while (len--) putchar(col, *str++);
	SpinLock_unlock(&_printLck);
    if (prevState) intr_unmask();
}

void clearScreen() {
	u64 prevState = intr_state();
	if (prevState) intr_mask();
    SpinLock_lock(&_printLck);
	memset(position.fbAddr, 0, (position.yPos + 1) * screen_charHeight * screen_info->pixelPreLine * sizeof(u32));
   	if (_bufAddr != NULL)
		memset(_bufAddr, 0, (position.yPos + 1) * screen_charHeight * screen_info->pixelPreLine * sizeof(u32));
	memset(_lineLen, 0, 4096 * sizeof(u32));
	position.xPos = 0, position.yPos = 0;
	SpinLock_unlock(&_printLck);
	if (prevState) intr_unmask();
}

void printk(u64 col, const char *fmt, ...) {
    char buf[512] = {0};
    int len = 0;
    va_list args;
    va_start(args, fmt);
    len = sprintfv(buf, fmt, args);
    va_end(args);
    _printStr(col, buf, len);
}

void printu(u64 col, const char *fmt, ...) {
    char buf[512] = {0};
    int len = 0;
    va_list args;
    va_start(args, fmt);
    len = sprintfv(buf, fmt, args);
    va_end(args);
    task_syscall3(task_syscall_print, col, buf, len);
}