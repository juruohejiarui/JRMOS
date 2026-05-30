#include <lib/string.h>

#define isDigit(ch) ((ch) >= '0' && (ch) <= '9')

static const unsigned int
	flag_fill_zero = 0x01, 
	flag_left = 0x02, 
	flag_space = 0x04, 
	flag_sign = 0x08, 
	flag_special = 0x10;

static __optimize__ int _skipAtoI(const char **fmt) {
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

static __optimize__ char *_number(char *str, i64 num, int base, int size, int precision, int tp) {
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
u64 sprintfv(char *buf, const char *fmt, va_list args) {
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

u64 sprintf(char *buf, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    u64 res = sprintfv(buf, fmt, args);
    va_end(args);
    return res;
}