#ifndef IOT_STDIO_H
#define IOT_STDIO_H

#include "stdarg.h"

//the argument of x must be an array not a point.
//so you have to modify the sprint to use snprntf() directly.
#define sprintf(x, fmt, arg...) snprintf(x, sizeof(x), fmt, ##arg)

// printfmt.c
void printfmt(void (*putch)(int, void*), void *putdat, const char *fmt, ...);
void vprintfmt(void (*putch)(int, void*), void *putdat, const char *fmt, va_list);
int snprintf(char *str, int size, const char *fmt, ...);
int vsnprintf(char *str, int size, const char *fmt, va_list);

// printf.c
int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list);

#ifdef IOT_PRINTF_DEBUG
#define iot_printf(fmt, arg...) printf(fmt, ##arg)
#else
#define iot_printf(fmt, arg...)
#endif

#define early_printf(fmt, arg...) printf(fmt, ##arg)

#endif /* !IOT_STDIO_H */
