#include <syscall.h>

int (*_putchar)(int c) = putchar;

int __attribute__((weak)) putchar(int c)
{
    return c;
}
