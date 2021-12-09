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
#ifdef BUILD_CORE_CORE0

#include "string.h"
#include "lib_dbglog.h"
#include "iot_memory_config.h"

#include "storage_controller.h"
#include "key_value.h"

#include "iot_gpio.h"
#include "iot_dma.h"
#include "iot_uart.h"
#include "iot_flash.h"

#include "generic_transmission_api.h"
#include "generic_transmission_io_flash.h"

/* flash log and ota use the same partition */
#define FLASH_LOG_LIMIT_SIZE (FLASH_FOTA_SECTORS*0x1000UL)
#define FLASH_LOG_OFFSET (uint32_t)(FLASH_FOTA_OFFSET - FLASH_START)

/* Log & Print configuration */
#define CONFIG_GENERIC_TRANSMISSION_IO_DEBUG                0
#if CONFIG_GENERIC_TRANSMISSION_IO_DEBUG
#define GENERIC_TRANSMISSION_IO_FLASH_LOGD(fmt, arg...)  iot_printf(fmt, ##arg)
#define GENERIC_TRANSMISSION_IO_FLASH_LOGI(fmt, arg...)  iot_printf(fmt, ##arg)
#define GENERIC_TRANSMISSION_IO_FLASH_LOGE(fmt, arg...)  iot_printf(fmt, ##arg)
#else
#define GENERIC_TRANSMISSION_IO_FLASH_LOGD(fmt, arg...)
#define GENERIC_TRANSMISSION_IO_FLASH_LOGE(fmt, arg...)
#endif

#define FLASH_LOG_ITEM_ID FLASH_LOG_BASE_ID

typedef struct flash_ctrl_t {
    uint32_t wd;
    uint32_t cover_flag;
} flash_ctrl;

//static function declare
static int32_t generic_transmission_io_flash_write(const uint8_t *buf, uint32_t len);
static int32_t generic_transmission_io_flash_write_panic(const uint8_t *buf, uint32_t len);

generic_transmission_io_method_t s_generic_transmission_io_flash_method = {
    .send = generic_transmission_io_flash_write,
    .send_panic = generic_transmission_io_flash_write_panic,
    .init = NULL,
    .deinit = NULL,
};

static int32_t generic_transmission_io_flash_update(const uint8_t *buf, uint32_t len, bool_t panic_flag)
{
    int ret = RET_OK;
    uint32_t param_len = sizeof(flash_ctrl);
    flash_ctrl ctrl_item;
    if (panic_flag) {
        uint8_t *log_ctrl_buf = NULL;
        if (key_value_read_key(FLASH_LOG_BASE_ID, &log_ctrl_buf, &param_len)) {
            ctrl_item.wd = 0;
            ctrl_item.cover_flag = 0;
            key_value_write_key(FLASH_LOG_BASE_ID, (uint8_t *)&ctrl_item, sizeof(flash_ctrl), true);
        } else {
            memcpy(&ctrl_item, log_ctrl_buf, param_len);
        }
    } else {
        if (RET_OK != storage_read(FLASH_LOG_BASE_ID, FLASH_LOG_ITEM_ID, (void *)&ctrl_item, &param_len)) {
            ctrl_item.wd = 0;
            ctrl_item.cover_flag = 0;
            storage_write(FLASH_LOG_BASE_ID, FLASH_LOG_ITEM_ID, (void *)&ctrl_item, (uint32_t)sizeof(flash_ctrl));
        }
    }
    assert(ctrl_item.wd < FLASH_LOG_LIMIT_SIZE);
    if (ctrl_item.wd + len > FLASH_LOG_LIMIT_SIZE) {
        uint32_t offset =  FLASH_LOG_LIMIT_SIZE - ctrl_item.wd;
        uint32_t remain_len = len - offset;
        ret |= iot_flash_write(FLASH_LOG_OFFSET + ctrl_item.wd, (void *)buf, offset);
        ret |= iot_flash_write(FLASH_LOG_OFFSET , (void *)(buf + offset), remain_len);
        if (RET_OK == ret) {
            // update flash_ctrl
            ctrl_item.wd = remain_len;
        } else {
            GENERIC_TRANSMISSION_IO_FLASH_LOGE("iot_flash_write error %d\n", ret);
        }
    } else {
        ret |= iot_flash_write(FLASH_LOG_OFFSET + ctrl_item.wd, (void *)buf, len);
        ctrl_item.wd += len;
        if (panic_flag) {
            ret |= key_value_write_key(FLASH_LOG_BASE_ID, (uint8_t *)&ctrl_item, sizeof(flash_ctrl), true);
        } else {
            ret |= (int)storage_write(FLASH_LOG_BASE_ID, FLASH_LOG_ITEM_ID, (void *)&ctrl_item, (uint32_t)sizeof(flash_ctrl));
         }
     }
     GENERIC_TRANSMISSION_IO_FLASH_LOGD(" ctrl wd %d ctrl cover_flag %d\n",ctrl_item.wd, ctrl_item.cover_flag)
     return ret;
}

static int32_t generic_transmission_io_flash_write(const uint8_t *buf, uint32_t len)
{
    return generic_transmission_io_flash_update(buf, len, false);
}

static int32_t generic_transmission_io_flash_write_panic(const uint8_t *buf, uint32_t len)
{
    if (IOT_FLASH_PE_SW_MODE == iot_flash_get_pe_mode()) {
        iot_flash_set_pe_mode(IOT_FLASH_PE_HW_MODE);
    }
    return generic_transmission_io_flash_update(buf, len, true);
}

/* This function here is for writing coredump data or error log to flash.
 */
void generic_transmission_io_flash_method_register(void)
{
    generic_transmission_io_method_register(GENERIC_TRANSMISSION_IO_FLASH, &s_generic_transmission_io_flash_method);
}

#endif /* BUILD_CORE_CORE0 */
