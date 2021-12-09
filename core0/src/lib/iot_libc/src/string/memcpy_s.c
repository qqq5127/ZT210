#include "string.h"

void* memcpy_s(void *dst, uint32_t dst_sz, const void *src, uint32_t len) IRAM_TEXT(memcpy_s);
void* memcpy_s(void *dst, uint32_t dst_sz, const void *src, uint32_t len)
{
    if (len > dst_sz) {
        uint32_t param[4];
        param[0] = dst_sz;
        param[1] = len;
        param[2] = (uint32_t)dst;
        param[3] = (uint32_t)src;

        // TODO: add assert dump
        UNUSED(param);
        assert(0);
    }

    return memcpy(dst, (void *)src, len);
}
