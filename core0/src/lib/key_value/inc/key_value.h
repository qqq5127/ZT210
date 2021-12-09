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
#ifndef KEY_VALUE_H
#define KEY_VALUE_H

#ifdef __cplusplus
extern "C" {
#endif

#define KV_DBG_EN 0

#if KV_DBG_EN
#include "stdio.h"
#define KV_DEBUG(fmt, argv...) printf(fmt, ##argv)
#else
#define KV_DEBUG(fmt, argv...)
#endif

#define KEY_VALUE_DEFRAG_PAGE_NUM 1
#define KEY_VALUE_MIN_PAGE_NUM    2
#define KEY_VALUE_PAGE_MAGIC      0x57714B76   // WqKv

#define KEY_VALUE_PAGE_SIZE 4096
#define KEY_VALUE_KEY_MAX_VALUE_LENGTH \
    ((KEY_VALUE_PAGE_SIZE - sizeof(kv_page_header_t)) - sizeof(kv_header_t))

#define KEY_VALUE_ADDR_ALIGN        0x04
#define KEY_VALUE_ADDR_ALIGN_MASK   0x03
#define KEY_VALUE_LENGTH_ALIGN      0x04
#define KEY_VALUE_LENGTH_ALIGN_MASK 0x03

#define KEY_VALUE_ALIGN(size, align)      (((size) + (align)-1) & ~((align)-1))
#define KEY_VALUE_ALIGN_DOWN(size, align) ((size) & ~((align)-1))

#define KV_VERSION 0x01

#define KV_PAGE_STATE_OFFSET     0x04
#define KV_PAGE_STATE_EMPTY      0xFFFFFFFF
#define KV_PAGE_STATE_USING      0xFFFFFF00
#define KV_PAGE_STATE_DEFRAGGING 0xFFFF0000

#define KV_PAGE_TYPE_NORMAL 0xFFFFFFFF
#define KV_PAGE_TYPE_DEFRAG 0xFFFFFF00

#define KV_KEY_STATE_UNUSED   0xFFFFFFFF
#define KV_KEY_STATE_WRITING  0xFFFFFF00
#define KV_KEY_STATE_VALID    0xFFFF0000
#define KV_KEY_STATE_DELETING 0xFF000000
#define KV_KEY_STATE_INVALID  0x00000000

typedef enum {
    KV_OK,
    KV_UNKONWN_ERROR,
    KV_NOT_INIT,
    KV_WRITE_ERROR,
    KV_READ_ERROR,
    KV_TOO_FEW_PAGES,
    KV_PAGE_NOT_FOUND,
    KV_KEY_NOT_FOUND,
    KV_FLASH_ERROR,
    KV_PAGE_STATE_ERROR,
    KV_KEY_STATE_ERROR,
    KV_PERMISSION_DENIED,
    KV_NO_ENOUGH_SPACE,
    KV_TOO_LONG,
    KV_NO_MEM,
} KV_ERROR;

typedef enum {
    KV_VALUE_UNUNED,
    KV_VALUE_WRITING,
    KV_VALUE_VALID,
    KV_VALUE_DELETING,
    KV_VALUE_INVALID,
} KV_VALUE_STATE;

typedef enum {
    KV_PAGE_EMPTY,
    KV_PAGE_USING,
    KV_PAGE_DEFRAGGING,
} KV_PAGE_STATE;

typedef struct kv_page_header {
    uint32_t type;   // page type flag
    uint32_t state;
    uint32_t magic;   // magic number
    uint16_t id;
    uint8_t version;
    uint8_t reserved;
} kv_page_header_t;

typedef struct kv_header {
    uint32_t state;
    uint32_t length;
    uint16_t id;
    uint8_t writeable;
    uint8_t reverved1;
} kv_header_t;

typedef struct key_value {
    kv_header_t header;
    uint8_t data[];
} key_value_t;

typedef struct kv_page_state {
    kv_page_header_t **empty;
    uint32_t empty_num;
    kv_page_header_t **using;
    uint32_t using_num;
    kv_page_header_t **defrag;
    uint32_t defrag_num;
} kv_page_state_t;

/**
 * @brief This function is used to defrag in key value.
 *
 * @return KV_OK for success otherwise error code.
 */
KV_ERROR key_value_defrag(void);

/**
 * @brief This function is used to recovery in key value.
 *
 * @return KV_ERROR
 */
KV_ERROR key_value_recovery(void);

/**
 * @brief This function is used to init in key value.
 *
 * @return KV_OK for success otherwise error code.
 */
KV_ERROR key_value_init(void);

/**
 * @brief This function is used to force reset in key value.
 *
 * @return KV_OK for success otherwise error code.
 */
KV_ERROR key_value_force_reset(void);

/**
 * @brief Write a key to flash
 * @note Thread unsafe
 *
 * @param id is struct key info's id.
 * @param data is write data.
 * @param length is the length of write data.
 * @param writeable is write or not.
 * @return KV_OK for success otherwise error code.
 */
KV_ERROR key_value_write_key(uint16_t id, const uint8_t *data, uint32_t length, bool_t writeable);

/**
 * @brief Read a key to flash
 * @note Thread unsafe
 *
 * @param id    Struct key info's id.
 * @param data  Pointer of data save pointer.
 * @param length    Key data length.
 * @return KV_OK for success otherwise error code.
 */
KV_ERROR key_value_read_key(uint16_t id, uint8_t **data, uint32_t *length);

/**
 * @brief Remove a key
 *
 * @param id is struct key info's id.
 * @return KV_OK for success otherwise error code.
 */
KV_ERROR key_value_del_key(uint16_t id);

/**
 * @brief The main API for set a key
 *
 * @param id is struct key info's id.
 * @param data is write data.
 * @param length is the length of write data.
 * @param writeable is write or not.
 * @return KV_OK for success otherwise error code.
 */
KV_ERROR key_value_set_key(uint16_t id, const uint8_t *data, uint32_t length, bool_t writeable);

/**
 * @brief This function is used to get key in key value.
 *
 * @param id is struct key info's id.
 * @param data is get kv value's data.
 * @param length is the length of get kv header data.
 * @return KV_OK for success otherwise error code.
 */
KV_ERROR key_value_get_key(uint16_t id, uint8_t **data, uint32_t *length);

/**
 * @brief This function is used to dump all keys in key value.
 */
void key_value_dump_all_keys(void);

#ifdef __cplusplus
}
#endif

#endif /* KEY_VALUE_H */
