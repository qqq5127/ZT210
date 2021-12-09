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
#ifndef STORAGE_CONTROLLER__H_
#define STORAGE_CONTROLLER__H_

#include "kv_id.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STORAGE_CONTROLLER_TEST_EN

typedef enum {
    IOMAP_BASE_ID = KV_ID_IOMAP_BASE,
    IOMAP_END_ID = 0x8FF,

    CONTROLLER_BASE_ID = 0xC00,
    CONTROLLER_END_ID = 0xCFF,

    HOST_BASE_ID = 0xD00,
    HOST_END_ID = 0xDFF,

    APP_BASE_ID = 0xE00,
    APP_END_ID = 0xEFF,

    LIB_BATTERY_BASE_ID = 0xF00,
    LIB_BATTERY_END_ID = 0xF0F,

    AUDIO_PARAM_BASE_ID = 0xF10,
    AUDIO_PARAM_END_ID = 0xF2F,

    AUDIO_CONTROL_BASE_ID = 0xF30,
    AUDIO_CONTROL_END_ID = 0xF3F,

    LIB_DBGLOG_BASE_ID = 0xF40,
    LIB_DBGLOG_END_ID = 0xF4F,

    LIB_DFS_BASE_ID = 0xF50,
    LIB_DFS_END_ID = 0xF5F,

    FRAGMENTARY_BASE_ID = 0xF60,
    FRAGMENTARY_END_ID = 0xF7F,

    VENDOR_BASE_ID = 0x1000,
    VENDOR_END_ID = 0x10FF,

    FLASH_LOG_BASE_ID = 0x2000,
    FLASH_LOG_END_ID = 0x20FF,

    //this region is reserved for oem access.
    OEM_BASE_ID = 0xFF80,
    OEM_END_ID = 0xFFFF,

    MAX_BASE_ID = OEM_END_ID,
} STORAGE_BASE_ID;

//IDs for oem access.
typedef enum {
    OEM_MAC_ID = OEM_BASE_ID,
    OEM_PPM_ID,
    OEM_IOMAP_ID,
    OEM_ANC_ID,
    OEM_MAX_ID = OEM_END_ID,
} STORAGE_OEM_ID;

void storage_init(void);

// /*  in param: id
//  *  in param: buf_addr: the pointer of the buffer
//  *  inout param: len_addr: the pointer of the length
//  *         in: len_addr: the size of the buffer
//  *         out: len_addr: the real length of the key in kv
//  *  retrun RET_OK: success, else failed
// */
/**
 * @brief This function is to read storage.
 *
 * @param module_id is the module base kv id.
 * @param id is the storage id.
 * @param buf is the pointer of the buffer.
 * @param p_len is the pointer of the length and will return length obtained from kv when ok.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t storage_read(uint32_t module_id, uint32_t id, void *buf, uint32_t *p_len);

// /*  in param: id
//  *  in param: buf_addr: the pointer of the buffer
//  *  in param: length:  the real length of the key
//  *  retrun RET_OK: success, else failed
// */
/**
 * @brief This function is to write storage.
 *
 * @param module_id is the module base kv id.
 * @param id is the storage id.
 * @param buf is the pointer of the buffer.
 * @param length is the pointer of the length.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t storage_write(uint32_t module_id, uint32_t id, void *buf, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* STORAGE_CONTROLLER__H_ */
