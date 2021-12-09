#ifndef _IOT_STRING_
#define _IOT_STRING_

#include <types.h>

size_t	strlen(const char *s);
size_t	strnlen(const char *s, size_t size);
char *	strcpy(char *dst, const char *src);
char *	strncpy(char *dst, const char *src, size_t size);
char *	strcat(char *dst, const char *src);
size_t	strlcpy(char *dst, const char *src, size_t size);
int	strcmp(const char *s1, const char *s2);
int	strncmp(const char *s1, const char *s2, size_t size);
char *	strchr(const char *s, int c);
char *	strfind(const char *s, char c);

void *	memset(void *dst, int c, size_t len);
void *	memcpy(void *dst, const void *src, size_t len);
void *	memmove(void *dst, const void *src, size_t len);
int	memcmp(const void *s1, const void *s2, size_t len);
void *	memfind(const void *s, int c, size_t len);
void* memcpy_s(void *dst, uint32_t dst_sz, const void *src, uint32_t len);
void* memset_s(void *ptr, uint32_t buf_sz, uint8_t value, uint32_t len);

long	strtol(const char *s, char **endptr, int base);

#endif
