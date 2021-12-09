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
#ifndef KEY_VALUE_PORT_H
#define KEY_VALUE_PORT_H

#include "chip_reg_base.h"
#include "iot_memory_origin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KV_OFFSET             FLASH_KEY_VALUE_OFFSET
#define KEY_VALUE_READ_ADDR  (FLASH_START + KV_OFFSET)
#define KEY_VALUE_WRITE_ADDR  KV_OFFSET
#define KEY_VALUE_PAGE_NUM         8

#ifndef CONFIG_KEY_VALUE_CACHE_ENABLE
#if defined(BUILD_OS_NON_OS)
#define CONFIG_KEY_VALUE_CACHE_ENABLE 0
#else
#define CONFIG_KEY_VALUE_CACHE_ENABLE 1
#endif
#endif

#define CONFIG_KEY_VALUE_CACHE_LENGTH 0x400

/**
 * @brief This function is used to write key value in flash.
 *
 * @param addr is the address of start.
 * @param data is the pointer of write data.
 * @param length is the length of write data.
 * @return int32_t RET_OK or -1.
 */
int32_t key_value_flash_write(uint32_t addr, const uint8_t *data, uint32_t length);

/**
 * @brief This function is used to erase page in flash.
 *
 * @param page is the page of start.
 * @return int32_t 0 or -1.
 */
int32_t key_value_page_erase(uint32_t page);

#ifdef __cplusplus
}
#endif

#endif /* KEY_VALUE_PORT_H */
