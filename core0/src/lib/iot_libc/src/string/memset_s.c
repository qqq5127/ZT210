#include "string.h"

void* memset_s(void *ptr, uint32_t buf_sz, uint8_t value, uint32_t len) IRAM_TEXT(memset_s);
void* memset_s(void *ptr, uint32_t buf_sz, uint8_t value, uint32_t len)
{
    if (len > buf_sz) {
        uint32_t param[4];
        param[0] = buf_sz;
        param[1] = len;
        param[2] = (uint32_t)ptr;
        param[3] = (uint32_t)value;

        // TODO: add assert dump
        UNUSED(param);
        assert(0);
    }

    return memset(ptr, value, len);
}
