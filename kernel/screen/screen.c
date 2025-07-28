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

#define isDigit(ch) ((ch) >= '0' && (ch) <= '9')

static const unsigned int
	flag_fill_zero = 0x01, 
	flag_left = 0x02, 
	flag_space = 0x04, 
	flag_sign = 0x08, 
	flag_special = 0x10;

static int _skipAtoI(const char **fmt) {
	int i = 0, sign = 1;
	if (**fmt == '-') sign = -1, (*fmt)++;
	while (isDigit(**fmt)) {
		i = i * 10 + *((*fmt)++) - '0';
	}
	return i * sign;
}

#define do_div(n, base) ({ \
    int __res; \
    __asm__("divq %%rcx" : "=a" (n), "=d" (__res) : "0" (n), "1" (0), "c" (base)); \
    __res; \
})

static char *_number(char *str, i64 num, int base, int size, int precision, int tp) {
    char c, sign, tmp[66];
    const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i;
    if (tp & flag_left) tp &= ~flag_fill_zero;
    if (base < 2 || base > 36) return 0;
    c = (tp & flag_fill_zero) ? '0' : ' ';
    sign = 0;
    if (tp & flag_sign) {
        if (num < 0) {
            sign = '-';
            num = -num;
            size--;
        } else if (tp & flag_space) {
            sign = ' ';
            size--;
        }
    }
    if (tp & flag_special) {
        if (base == 16) {
            *str++ = '0';
            *str++ = 'x';
            size -= 2;
        } else if (base == 8) *str++ = '0', size--;
    }
    i = 0;
    if (num == 0) tmp[i++] = '0';
    else while (num != 0) tmp[i++] = digits[do_div(num, base)];
    if (i > precision) precision = i;
    size -= precision;
    if (!(tp & (flag_left | flag_fill_zero))) while (size-- > 0) *str++ = ' ';
    if (sign) *str++ = sign;
    if (!(tp & flag_left)) while (size-- > 0) *str++ = c;
    while (i < precision--) *str++ = '0';
    while (i-- > 0) *str++ = tmp[i];
    while (size-- > 0) *str++ = ' ';
    return str;
}
// get the string using FMT and ARGS, output to BUF and return the length of the output
__always_inline__ int _sprintf(char *buf, const char *fmt, va_list args) {
    char *str = buf, *s;
    int flags, len, qlf, i, fld_w, prec;
    while (*fmt != '\0') {
        if (*fmt != '%') {
            *(str++) = *(fmt++);
        } else {
            fmt++, flags = 0, len = 0;
            if (*fmt == '%') {
                *(str++) = *(fmt++);
                continue;
            }
            scanFlags:
            switch (*fmt) {
                case '0': flags |= flag_fill_zero; fmt++; goto scanFlags;
                case '-': flags |= flag_left; fmt++; goto scanFlags;
                case ' ': flags |= flag_space; fmt++; goto scanFlags;
                case '+': flags |= flag_sign; fmt++; goto scanFlags;
                case '#': flags |= flag_special; fmt++; goto scanFlags;
                default: goto endScanFlags;
            }
            endScanFlags:
            fld_w = -1, qlf = 0;
            if (isDigit(*fmt))
                fld_w = _skipAtoI(&fmt);
            else if (*fmt == '*') {
                fld_w = va_arg(args, int);
                fmt++;
                if (fld_w < 0)
                    fld_w = -fld_w, flags |= flag_left;
            }
            prec = -1;
            if (*fmt == '.') {
                fmt++;
                if (isDigit(*fmt))
                    prec = _skipAtoI(&fmt);
                else if (*fmt == '*') {
                    prec = va_arg(args, int);
                    fmt++;
                }
                if (prec < 0) prec = 0;
            }
            switch (*fmt) {
                case 'h': qlf = (*(fmt + 1) == 'h' ? (fmt += 2, 'H') : (fmt++, 'h')); break;
                case 'l': qlf = (*(fmt + 1) == 'l' ? (fmt += 2, 'L') : (fmt++, 'L')); break;
                case 'j': qlf = (fmt++, 'j'); break;
                case 'z': qlf = (fmt++, 'z'); break;
                case 't': qlf = (fmt++, 't'); break;
                case 'L': qlf = (fmt++, 'L'); break;
                default: qlf = 0; break;
            }
            switch (*fmt) {
                case 'c':
                    if (!(flags & flag_left))
                        while (--fld_w > 0) *(str++) = ' ';
                    *(str++) = (unsigned char)va_arg(args, int);
                    while (--fld_w > 0) *(str++) = ' ';
                    break;
                // support width string (print the low byte)
                case 'S':
                    s = va_arg(args, char *);
                    len = 0;
                    while (s[len]) len += 2;
                    if (prec >= 0 && len / 2 > prec) len = prec * 2;
                    if (!(flags & flag_left))
                        while (len < fld_w--) *(str++) = ' ';
                    for (i = 0; i < len; i += 2) *(str++) = *(s + i);
                    while (len / 2 < fld_w--) *(str++) = ' ';
                    break;
                case 's':
                    s = va_arg(args, char *);
                    len = strlen(s);
                    if (prec >= 0 && len > prec) len = prec;
                    if (!(flags & flag_left))
                        while (len < fld_w--) *(str++) = ' ';
                    for (i = 0; i < len; i++) *(str++) = *(s + i);
                    while (len < fld_w--) *(str++) = ' ';
                    break;
                case 'o':
                    str = _number(str, qlf == 'L' ? va_arg(args, u64) : va_arg(args, u32), 8, fld_w, prec, flags);
                    break;
                case 'p':
                    if (fld_w == -1) {
                        fld_w = 2 * sizeof(void *) + 2;
                        flags |= flag_fill_zero;
                        flags |= flag_special;
                    }
                    str = _number(str, (unsigned long)va_arg(args, void *), 16, fld_w, prec, flags);
                    break;
                case 'x':
                case 'X':
                    str = _number(str, qlf == 'L' ? va_arg(args, u64) : va_arg(args, u32), 16, fld_w, prec, flags);
                    break;
                case 'd':
                case 'i':
                    flags |= flag_sign;
                    str = _number(str, qlf == 'L' ? va_arg(args, i64) : va_arg(args, i32), 10, fld_w, prec, flags);
					break;
                case 'u':
                    str = _number(str, qlf == 'L' ? va_arg(args, u64) : va_arg(args, u32), 10, fld_w, prec, flags);
                    break;
                default:
                    *(str++) = '%';
                    break;
            }
            *fmt++;
        }
    }
    *str = '\0';
    return str - buf;
}

static void _scroll(void) {
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

static void _drawchar(unsigned int fcol, unsigned int bcol, int px, int py, char ch) {
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

void putchar(u64 col, char ch) {
    int i;
    if (ch == '\n') {
        position.yPos++, position.xPos = 0;
        if (position.yPos >= min(64, position.yRes / screen_charHeight)) {
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
        if (position.xPos == position.xRes / screen_charWidth)
            putchar(col, '\n');
        _drawchar(col & 0xffffffff, (col >> 32) & 0xffffffff, screen_charWidth * position.xPos, screen_charHeight * position.yPos, ch);
        position.xPos++;
        _lineLen[position.yPos] = max(_lineLen[position.yPos], position.xPos);
    }
}

static void _printStr(u64 col, const char *str, int len) {
    // close the interrupt if it is open now
	u64 prevState = intr_state();
	if (prevState) intr_mask();
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
    len = _sprintf(buf, fmt, args);
    va_end(args);
    _printStr(col, buf, len);
}

void printu(u64 col, const char *fmt, ...) {
    char buf[512] = {0};
    int len = 0;
    va_list args;
    va_start(args, fmt);
    len = _sprintf(buf, fmt, args);
    va_end(args);
    task_syscall3(task_syscall_print, col, buf, len);
}