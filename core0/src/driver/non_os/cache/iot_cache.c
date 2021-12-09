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
#include "iot_cache.h"
#include "iot_flash.h"

#include "sfc.h"
#ifdef LOW_POWER_ENABLE
#include "dev_pm.h"
#endif

static bool_t restore_done = false;

void iot_cache_init(uint8_t cache_id)
{
    restore_done = false;
    if (APB_CACHE_SFC_ID == cache_id) {
        if (!iot_flash_is_init()) {
            iot_flash_init();
        }
        iot_flash_enable_quad_mode();
        iot_flash_set_cache_mode();
    }

    apb_cache_init(cache_id);
}

void iot_cache_deinit(uint8_t cache_id)
{
    apb_cache_deinit(cache_id);
}

void iot_cache_config_as_ram_mode(uint8_t cache_id)
{
    apb_cache_enable_ram(cache_id);
}

void iot_cache_clear(uint8_t cache_id)
{
    apb_cache_clear(cache_id);
}

void iot_cache_flush(uint8_t cache_id, uint32_t addr, uint32_t len)
{
    apb_cache_flush(cache_id, addr, len);
}

void iot_cache_invalidate(uint8_t cache_id, uint32_t addr, uint32_t len)
{
    apb_cache_invalidate(cache_id, addr, len);
}

void iot_cache_miss_clear(uint8_t cache_id)
{
    apb_cache_miss_clear(cache_id);
}

void iot_cache_miss_enable(uint8_t cache_id, bool_t en)
{
    apb_cache_miss_enable(cache_id, en);
}

uint32_t iot_cache_get_miss_cnt(uint8_t cache_id)
{
    static uint8_t init = 0;
    uint32_t cnt;

    if (!init) {
        apb_cache_miss_enable(cache_id, true);
        init = 1;
    }
    cnt = apb_cache_miss_cnt(cache_id);

    apb_cache_miss_clear(cache_id);

    return cnt;
}

#ifdef LOW_POWER_ENABLE

static uint32_t sfc_default_io_map = 0;

static uint32_t iot_cache_save(uint32_t arg);
static uint32_t iot_cache_restore(uint32_t arg);

static struct pm_operation cache_node = {
    .node = LIST_HEAD_INIT(cache_node.node),
    .save = iot_cache_save,
    .restore = iot_cache_restore,
};

static uint32_t iot_cache_save(uint32_t arg)
{
    UNUSED(arg);
    restore_done = false;
    sfc_default_io_map = sfc_get_io_map();
    return RET_OK;
}

static void cache_flash_init(void) IRAM_TEXT(cache_flash_init);
static void cache_flash_init(void)
{
    extern void sfc_init(void);
    extern void sfc_set_io_map(uint32_t map);
    sfc_init();
    sfc_set_io_map(sfc_default_io_map);
}

static uint32_t iot_cache_restore(uint32_t arg) IRAM_TEXT(iot_cache_restore);
static uint32_t iot_cache_restore(uint32_t arg)
{
    UNUSED(arg);

    cache_flash_init();
    iot_flash_enable_quad_mode();
    iot_flash_set_cache_mode();

    apb_cache_init(APB_CACHE_SFC_ID);
    apb_cache_miss_enable(APB_CACHE_SFC_ID, true);
    restore_done = true;

    return RET_OK;
}

void iot_cache_restore_register(void)
{
    iot_dev_pm_register(&cache_node);
}

bool_t iot_cache_get_restore_status(void) IRAM_TEXT(iot_cache_get_restore_status);
bool_t iot_cache_get_restore_status(void)
{
    return restore_done;
}
#endif
