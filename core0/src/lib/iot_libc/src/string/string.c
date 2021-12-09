#include <string.h>

size_t strnlen(const char *s, size_t size) IRAM_TEXT(strnlen);
size_t strnlen(const char *s, size_t size)
{
    int n;

    for (n = 0; size > 0 && *s != '\0'; s++, size--)
        n++;
    return n;
}

char *strcat(char *dst, const char *src) IRAM_TEXT(strcat);
char *strcat(char *dst, const char *src)
{
    int len = strlen(dst);
    strcpy(dst + len, src);
    return dst;
}

char *strncpy(char *dst, const char *src, size_t size) IRAM_TEXT(strncpy);
char *strncpy(char *dst, const char *src, size_t size)
{
    size_t i;
    char *ret;

    ret = dst;
    for (i = 0; i < size; i++) {
        *dst++ = *src;
        // If strlen(src) < size, null-pad 'dst' out to 'size' chars
        if (*src != '\0')
            src++;
    }
    return ret;
}

size_t strlcpy(char *dst, const char *src, size_t size) IRAM_TEXT(strlcpy);
size_t strlcpy(char *dst, const char *src, size_t size)
{
    char *dst_in;

    dst_in = dst;
    if (size > 0) {
        while (--size > 0 && *src != '\0')
            *dst++ = *src++;
        *dst = '\0';
    }
    return dst - dst_in;
}

int strncmp(const char *p, const char *q, size_t n) IRAM_TEXT(strncmp);
int strncmp(const char *p, const char *q, size_t n)
{
    while (n > 0 && *p && *p == *q)
        n--, p++, q++;
    if (n == 0)
        return 0;
    else
        return (int) ((unsigned char) *p - (unsigned char) *q);
}

// Return a pointer to the first occurrence of 'c' in 's',
// or a null pointer if the string has no 'c'.
char *strchr(const char *s, int c) IRAM_TEXT(strchr);
char *strchr(const char *s, int c)
{
    for (; *s; s++)
        if (*s == c)
            return (char *) s;
    return 0;
}

// Return a pointer to the first occurrence of 'c' in 's',
// or a pointer to the string-ending null character if the string has no 'c'.
char *strfind(const char *s, char c) IRAM_TEXT(strfind);
char *strfind(const char *s, char c)
{
    for (; *s; s++)
        if (*s == c)
            break;
    return (char *) s;
}

void *memmove(void *dst, const void *src, size_t n) IRAM_TEXT(memmove);
void *memmove(void *dst, const void *src, size_t n)
{
    const char *s;
    char *d;

    s = src;
    d = dst;
    if (s < d && s + n > d) {
        s += n;
        d += n;
        while (n-- > 0)
            *--d = *--s;
    } else
        while (n-- > 0)
            *d++ = *s++;

    return dst;
}

int memcmp(const void *v1, const void *v2, size_t n) IRAM_TEXT(memcmp);
int memcmp(const void *v1, const void *v2, size_t n)
{
    const uint8_t *s1 = (const uint8_t *) v1;
    const uint8_t *s2 = (const uint8_t *) v2;

    while (n-- > 0) {
        if (*s1 != *s2)
            return (int) *s1 - (int) *s2;
        s1++, s2++;
    }

    return 0;
}

void *memfind(const void *s, int c, size_t n) IRAM_TEXT(memfind);
void *memfind(const void *s, int c, size_t n)
{
    const void *ends = (const char *) s + n;
    for (; s < ends; s = (const char *)s + 1)
        if (*(const unsigned char *) s == (unsigned char) c)
            break;
    return (void *) s;
}

long strtol(const char *s, char **endptr, int base) IRAM_TEXT(strtol);
long strtol(const char *s, char **endptr, int base)
{
    int neg = 0;
    long val = 0;

    // gobble initial whitespace
    while (*s == ' ' || *s == '\t')
        s++;

    // plus/minus sign
    if (*s == '+')
        s++;
    else if (*s == '-')
        s++, neg = 1;

    // hex or octal base prefix
    if ((base == 0 || base == 16) && (s[0] == '0' && s[1] == 'x'))
        s += 2, base = 16;
    else if (base == 0 && s[0] == '0')
        s++, base = 8;
    else if (base == 0)
        base = 10;

    // digits
    while (1) {
        int dig;

        if (*s >= '0' && *s <= '9')
            dig = *s - '0';
        else if (*s >= 'a' && *s <= 'z')
            dig = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'Z')
            dig = *s - 'A' + 10;
        else
            break;
        if (dig >= base)
            break;
        s++, val = (val * base) + dig;
        // we don't properly detect overflow!
    }

    if (endptr)
        *endptr = (char *) s;
    return (neg ? -val : val);
}
