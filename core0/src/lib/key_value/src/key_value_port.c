/****************************************************************************

Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
/* os shim includes */
#include "types.h"
#include "string.h"

#include "iot_flash.h"
#include "key_value_port.h"
#include "key_value.h"
#include "iot_cache.h"

#define ENABLE_WRITE_CHECK 1

int32_t key_value_flash_write(uint32_t addr, const uint8_t *data, uint32_t length)
{
    uint32_t flash_addr;
    int32_t ret;

    assert(length < KEY_VALUE_KEY_MAX_VALUE_LENGTH);

    if ((addr < KEY_VALUE_READ_ADDR)
        || addr > (KEY_VALUE_READ_ADDR
                   + KEY_VALUE_PAGE_SIZE * KEY_VALUE_PAGE_NUM)) {
        return -1;
    }

    flash_addr = (addr - KEY_VALUE_READ_ADDR) + KEY_VALUE_WRITE_ADDR;

    KV_DEBUG(
        "key_value_flash_write: flash_offset: 0x%x, length=%d, data:0x%x \n",
        flash_addr, length, *(uint32_t *)data);

    ret = iot_flash_write_without_erase(flash_addr, data, length);

    return ret;
}

int32_t key_value_page_erase(uint32_t addr)
{
    uint32_t page_start;
    uint32_t flash_addr;

    page_start = KEY_VALUE_ALIGN_DOWN(addr, KEY_VALUE_PAGE_SIZE);
    flash_addr = (page_start - KEY_VALUE_READ_ADDR) + KEY_VALUE_WRITE_ADDR;

    if (iot_flash_erase(flash_addr)) {
        KV_DEBUG("[KV]Erase page 0x%08x error.\n", page_start);
        return -1;
    }
    KV_DEBUG("[KV]Erase page 0x%08x ok.\n", page_start);

#if ENABLE_WRITE_CHECK
    for (uint32_t i = 0; i < KEY_VALUE_PAGE_SIZE; i += 4) {
        if (*(volatile uint32_t *)page_start != 0xFFFFFFFF) {
            KV_DEBUG("[KV]Erase error %p:0x%08x\n", (void *)page_start,
                     *(volatile uint32_t *)page_start);
            assert(0);
        }
    }
#endif

    return 0;
}
