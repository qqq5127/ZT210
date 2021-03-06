// Stripped-down primitive printf-style formatting routines,
// used in common by printf, sprintf, fprintf, etc.
// This code is also used by both the kernel and user programs.

#include <types.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <error.h>

/*
 * Space or zero padding and a field width are supported for the numeric
 * formats only.
 *
 * The special format %e takes an integer error code
 * and prints a string describing the error.
 * The integer may be positive or negative,
 * so that -E_NO_MEM and E_NO_MEM are equivalent.
 */

static const char * const error_string[MAXERROR] = {
    [E_UNSPECIFIED]  = "unspecified error",
};

/*
 * Print a number (base <= 16) in reverse order,
 * using specified putch function and associated pointer putdat.
 * Return width of print number.
 */
static int
printnum(void (*putch)(int, void*), void *putdat,
        unsigned long long num, unsigned base, int width, int padc) IRAM_TEXT(printnum);
static int
printnum(void (*putch)(int, void*), void *putdat,
        unsigned long long num, unsigned base, int width, int padc)
{
    int w = 0;

    // first recursively print all preceding (more significant) digits
    if (num >= base) {
        w = printnum(putch, putdat, num / base, base, width - 1, padc);
    } else {
        // print any needed pad characters before first digit
        while (--width > 0 && padc != '-') {
            putch(padc, putdat);
            w++;
        }
    }

    // then print this (the least significant) digit
    putch("0123456789abcdef"[num % base], putdat);
    return w + 1;
}

static inline long long pow10(int t) IRAM_TEXT(pow10);
static inline long long pow10(int t)
{
    long long p = 1;

    while (t--)
        p *= 10;
    return p;
}

static void
printdouble(void (*putch)(int, void*), void *putdat, double num,
            int width, int precision, int padc) IRAM_TEXT(printdouble);
static void
printdouble(void (*putch)(int, void*), void *putdat, double num,
            int width, int precision, int padc)
{
    long long inte;
    long long deci;

    // default precision
    if (precision < 0)
        precision = 6;

    inte = (long long)num;
    deci = (long long)(pow10(precision + 1) * (num - inte));

    if (deci % 10 >= 5)
        deci = deci / 10 + 1;
    else
        deci = deci / 10;

    printnum(putch, putdat, inte, 10, width, padc);
    putch('.', putdat);
    printnum(putch, putdat, deci, 10, precision, '0');
    return;
}

// Get an unsigned int of various possible sizes from a varargs list,
// depending on the lflag parameter.
static unsigned long long
getuint(va_list *ap, int lflag) IRAM_TEXT(getuint);
static unsigned long long
getuint(va_list *ap, int lflag)
{
    if (lflag >= 2)
        return va_arg(*ap, unsigned long long);
    else if (lflag)
        return va_arg(*ap, unsigned long);
    else
        return va_arg(*ap, unsigned int);
}

// Same as getuint but signed - can't use getuint
// because of sign extension
static long long
getint(va_list *ap, int lflag) IRAM_TEXT(getint);
static long long
getint(va_list *ap, int lflag)
{
    if (lflag >= 2)
        return va_arg(*ap, long long);
    else if (lflag)
        return va_arg(*ap, long);
    else
        return va_arg(*ap, int);
}

void
vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list ap) IRAM_TEXT(vprintfmt);
void
vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list ap)
{
    register const char *p;
    register int ch, err;
    unsigned long long num;
    double float_num;
    int base, lflag, width, precision, altflag, num_width;
    char padc;

    while (1) {
        while ((ch = *(unsigned char *) fmt++) != '%') {
            if (ch == '\0')
                return;
            putch(ch, putdat);
            if (ch == '\n')
                putch('\r', putdat);
        }

        // Process a %-escape sequence
        padc = ' ';
        width = -1;
        precision = -1;
        lflag = 0;
        altflag = 0;
    reswitch:
        switch (ch = *(unsigned char *) fmt++) {

        // flag to pad on the right
        case '-':
            padc = '-';
            goto reswitch;

        // flag to pad with 0's instead of spaces
        case '0':
            padc = '0';
            goto reswitch;

        // width field
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            for (precision = 0; ; ++fmt) {
                precision = precision * 10 + ch - '0';
                ch = *fmt;
                if (ch < '0' || ch > '9')
                    break;
            }
            goto process_precision;

        case '*':
            precision = va_arg(ap, int);
            goto process_precision;

        case '.':
            if (width < 0)
                width = 0;
            goto reswitch;

        case '#':
            altflag = 1;
            goto reswitch;

        process_precision:
            if (width < 0)
                width = precision, precision = -1;
            goto reswitch;

        // long flag (doubled for long long)
        case 'l':
            lflag++;
            goto reswitch;

        // character
        case 'c':
            putch(va_arg(ap, int), putdat);
            break;

        // error message
        case 'e':
            err = va_arg(ap, int);
            if (err < 0)
                err = -err;
            if (err >= MAXERROR || (p = error_string[err]) == NULL)
                printfmt(putch, putdat, "error %d", err);
            else
                printfmt(putch, putdat, "%s", p);
            break;

        // string
        case 's':
            if ((p = va_arg(ap, char *)) == NULL)
                p = "(null)";
            if (width > 0 && padc != '-')
                for (width -= strnlen(p, precision); width > 0; width--)
                    putch(padc, putdat);
            for (; (ch = *p++) != '\0' && (precision < 0 || --precision >= 0); width--)
                if (altflag && (ch < ' ' || ch > '~'))
                    putch('?', putdat);
                else
                    putch(ch, putdat);
            for (; width > 0; width--)
                putch(' ', putdat);
            break;

        // (signed) decimal
        case 'd':
            num = getint(&ap, lflag);
            if ((long long) num < 0) {
                putch('-', putdat);
                num = -(long long) num;
            }
            base = 10;
            goto number;

        // unsigned decimal
        case 'u':
            num = getuint(&ap, lflag);
            base = 10;
            goto number;

        // (unsigned) octal
        case 'o':
            num = getuint(&ap, lflag);
            base = 8;
            goto number;

        // pointer
        case 'p':
            putch('0', putdat);
            putch('x', putdat);
            num = (unsigned long long)
                (uintptr_t) va_arg(ap, void *);
            base = 16;
            goto number;

        // (unsigned) hexadecimal
        case 'x':
        case 'X':
            num = getuint(&ap, lflag);
            base = 16;
        number:
            num_width = printnum(putch, putdat, num, base, width, padc);
            for (; width - num_width > 0; width--)
                putch(' ', putdat);
            break;

        // float
        case 'f':
            float_num = va_arg(ap, double);
            if (float_num < 0) {
                putch('-', putdat);
                float_num = -float_num;
            }
            printdouble(putch, putdat, float_num, width, precision, padc);
            break;

        // escaped '%' character
        case '%':
            putch(ch, putdat);
            break;

        // unrecognized escape sequence - just print it literally
        default:
            putch('%', putdat);
            for (fmt--; fmt[-1] != '%'; fmt--)
                /* do nothing */;
            break;
        }
    }
}
// Main function to format and print a string.
void printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...) IRAM_TEXT(printfmt);
void printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintfmt(putch, putdat, fmt, ap);
    va_end(ap);
}

struct sprintbuf {
    char *buf;
    char *ebuf;
    int cnt;
};

static void sprintputch(int ch, struct sprintbuf *b) IRAM_TEXT(sprintputch);
static void sprintputch(int ch, struct sprintbuf *b)
{
    b->cnt++;
    if (b->buf < b->ebuf)
        *b->buf++ = ch;
}

int vsnprintf(char *buf, int n, const char *fmt, va_list ap) IRAM_TEXT(vsnprintf);
int vsnprintf(char *buf, int n, const char *fmt, va_list ap)
{
    struct sprintbuf b = {buf, buf+n-1, 0};

    if (buf == NULL || n < 1)
        return -E_INVAL;

    // print the string to the buffer
    vprintfmt((void*)sprintputch, &b, fmt, ap);

    // null terminate the buffer
    *b.buf = '\0';

    return b.cnt;
}

int snprintf(char *buf, int n, const char *fmt, ...) IRAM_TEXT(snprintf);
int snprintf(char *buf, int n, const char *fmt, ...)
{
    va_list ap;
    int rc;

    va_start(ap, fmt);
    rc = vsnprintf(buf, n, fmt, ap);
    va_end(ap);

    return rc;
}
