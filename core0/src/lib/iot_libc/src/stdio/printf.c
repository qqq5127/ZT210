#include <types.h>
#include <stdio.h>
#include <stdarg.h>
#include <syscall.h>

static void putch(int ch, int *cnt) IRAM_TEXT(putch);
static void putch(int ch, int *cnt)
{
    _putchar(ch);
    ++(*cnt);
}

int vprintf(const char *fmt, va_list ap) IRAM_TEXT(vprintf);
int vprintf(const char *fmt, va_list ap)
{
    int cnt = 0;
    vprintfmt((void *)putch, &cnt, fmt, ap);
    return cnt;
}

int printf(const char *fmt, ...) IRAM_TEXT(printf);
int printf(const char *fmt, ...)
{
    va_list ap;
    int cnt;

    /* set ap as (&fmt + 1) */
    va_start(ap, fmt);
    cnt = vprintf(fmt, ap);
    /* set ap as NULL */
    va_end(ap);

    return cnt;
}
