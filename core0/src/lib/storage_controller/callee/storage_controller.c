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
#include "types.h"
#include "string.h"   // string definitions
#include "storage_controller.h"
#include "key_value.h"
#include "os_event.h"

#include "os_lock.h"
#include "oem.h"

static os_mutex_h storage_mutex = NULL;

/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */
/**
 * @brief This function is to check whether id is legal.
 *
 * @param module_id is the module base kv id.
 * @param id is the storage id.
 * @return uint8_t RET_OK for success else error.
 */
static uint8_t storage_controller_id_check(uint32_t module_id, uint32_t id);

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
static uint8_t kv_read(uint32_t id, void *buf, uint32_t *p_len)
{
    uint8_t status;
    uint8_t *data;
    uint32_t len = 0;

    os_acquire_mutex(storage_mutex);
    status = key_value_get_key((uint16_t)id, &data, &len);
    if (status == RET_OK) {
        if ((buf == NULL) || (p_len == NULL) || (*p_len == 0)) {
            assert(0);
        }

        if (len > *p_len) {
            memcpy((uint8_t *)buf, data, *p_len);
            os_release_mutex(storage_mutex);
            return RET_NOMEM;
        }

        memcpy((uint8_t *)buf, data, len);
        *p_len = len;
    }
    os_release_mutex(storage_mutex);
    return status;
}

static uint8_t oem_read(uint32_t id, void *buf, uint32_t *p_len)
{
    uint8_t status = RET_FAIL;

    os_acquire_mutex(storage_mutex);
    switch (id) {
        case OEM_MAC_ID:
            assert(*p_len >= MAC_ADDR_LEN);
            status = oem_data_get_mac_addr((uint8_t *)buf);
            *p_len = MAC_ADDR_LEN;
            break;
        case OEM_PPM_ID:
            assert(*p_len >= sizeof(uint8_t));
            status = oem_data_get_ppm(buf);
            *p_len = sizeof(uint8_t);
            break;
        case OEM_IOMAP_ID:
        case OEM_ANC_ID:
            //TODO, this two ids are not available. Please use oem API.
            break;
        default:
            break;
    }
    os_release_mutex(storage_mutex);

    return status;
}

void storage_init(void)
{
    if (storage_mutex == NULL) {
        storage_mutex = os_create_mutex(STORAGE_MID);
        assert(storage_mutex);
    }
}

uint32_t storage_read(uint32_t module_id, uint32_t id, void *buf, uint32_t *p_len)
{
    uint8_t status;

    assert(buf);

    status = storage_controller_id_check(module_id, id);
    if (status != RET_OK) {
        return status;
    }

    //KV access
    if (module_id < OEM_BASE_ID) {
        status = kv_read((uint16_t)id, buf, p_len);
    }
    //oem access
    else {
        status = oem_read(id, buf, p_len);
    }
    return status;
}

uint32_t storage_write(uint32_t module_id, uint32_t id, void *buf, uint32_t length)
{
    uint8_t status;
    uint32_t len = length;

    status = storage_controller_id_check(module_id, id);
    if (status != RET_OK) {
        return status;
    }

    if (buf == NULL) {
        assert(0);
    }
    //KV access
    if (module_id < OEM_BASE_ID) {
        os_acquire_mutex(storage_mutex);
        status = key_value_set_key((uint16_t)id, (uint8_t *)buf, len, true);
        os_release_mutex(storage_mutex);
    }
    //oem access
    else {
        //TODO: oem write using storage lib is not enabled.
        assert(0);
    }
    return status;
} /*lint !e818 rpc function do not declared as pointing to const */

static uint8_t storage_controller_id_check(uint32_t module_id, uint32_t id)
{
    uint8_t status = RET_INVAL;

    switch (module_id) {
        case IOMAP_BASE_ID:
            if ((id >= IOMAP_BASE_ID) && (id <= IOMAP_END_ID)) {
                status = RET_OK;
            }

            break;
        case CONTROLLER_BASE_ID:
            if ((id >= CONTROLLER_BASE_ID) && (id <= CONTROLLER_END_ID)) {
                status = RET_OK;
            }

            break;
        case HOST_BASE_ID:
            if ((id >= HOST_BASE_ID) && (id <= HOST_END_ID)) {
                status = RET_OK;
            }

            break;
        case APP_BASE_ID:
            if ((id >= APP_BASE_ID) && (id <= APP_END_ID)) {
                status = RET_OK;
            }

            break;
        case LIB_BATTERY_BASE_ID:
            if ((id >= LIB_BATTERY_BASE_ID) && (id <= LIB_BATTERY_END_ID)) {
                status = RET_OK;
            }
            break;

        case AUDIO_PARAM_BASE_ID:
            if ((id >= AUDIO_PARAM_BASE_ID) && (id <= AUDIO_PARAM_END_ID)) {
                status = RET_OK;
            }
            break;
        case AUDIO_CONTROL_BASE_ID:
            if ((id >= AUDIO_CONTROL_BASE_ID) && (id <= AUDIO_CONTROL_END_ID)) {
                status = RET_OK;
            }
            break;

        case LIB_DBGLOG_BASE_ID:
            if ((id >= LIB_DBGLOG_BASE_ID) && (id <= LIB_DBGLOG_END_ID)) {
                status = RET_OK;
            }
            break;

        case LIB_DFS_BASE_ID:
            if ((id >= LIB_DFS_BASE_ID) && (id <= LIB_DFS_END_ID)) {
                status = RET_OK;
            }
            break;
        case FRAGMENTARY_BASE_ID:
            if ((id >= FRAGMENTARY_BASE_ID) && (id <= FRAGMENTARY_END_ID)) {
                status = RET_OK;
            }
            break;
        case VENDOR_BASE_ID:
            if ((id >= VENDOR_BASE_ID) && (id <= VENDOR_END_ID)) {
                status = RET_OK;
            }
            break;
        case FLASH_LOG_BASE_ID:
            if ((id >= FLASH_LOG_BASE_ID) && (id <= FLASH_LOG_END_ID)) {
                status = RET_OK;
            }
            break;
        case OEM_BASE_ID:
            if ((id >= OEM_BASE_ID) && (id <= OEM_END_ID)) {
                status = RET_OK;
            }

            break;
        default:
            break;
    }

    return status;
}
