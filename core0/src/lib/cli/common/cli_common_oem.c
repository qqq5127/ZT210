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
#include "string.h"
#include "crc.h"

#include "iot_flash.h"
#include "iot_cache.h"
#include "iot_soc.h"
#include "oem.h"

#include "lib_dbglog.h"
#include "dbglog.h"

#include "cli.h"
#include "cli_common_definition.h"
#include "cli_common_basic.h"
#include "cli_common_oem.h"
#include "storage_controller.h"
#include "fragmentary_kv.h"


static uint8_t cli_oem_data_base_save(oem_data_base_t *data)
{
    uint8_t ret = iot_flash_write(OEM_DATA_BASE_OFFSET, (uint8_t *)data, sizeof(oem_data_base_t));

    //invalidate cache;
    iot_cache_invalidate(IOT_CACHE_SFC_ID, FLASH_START + OEM_DATA_BASE_OFFSET,
                         sizeof(oem_data_base_t));

    return ret;
}

static void cli_oem_data_set_ppm(uint32_t ppm)
{
    uint8_t status = RET_FAIL;
    uint32_t len = 0;
    uint8_t *data;
    oem_data_base_t base;
    oem_data_base_data_t *oem_data = (oem_data_base_data_t *)&base.data;

    uint32_t ret;
    ppm_adjust_kv_cfg_t kv_cfg;
    uint32_t kv_cfg_len = sizeof(ppm_adjust_kv_cfg_t);

    memset((uint8_t *)&base, 0x0, sizeof(oem_data_base_t));

    status = oem_data_base_load(&data, &len);
    if (status == RET_OK) {
        memcpy((uint8_t *)oem_data, data, len);
    }
    oem_data->ppm = ppm;

    ret = storage_read(FRAGMENTARY_BASE_ID, PPM_ADJUST_ID,
                 (void *)&kv_cfg,  &kv_cfg_len);

    if (ret != RET_OK || kv_cfg_len != sizeof(ppm_adjust_kv_cfg_t)) {
        DBGLOG_LIB_ERROR("[ppm adjust] Read KV err: %d, len %d\n", ret, kv_cfg_len);
    } else {
        oem_data->ppm += kv_cfg.ppm_adjust;
        DBGLOG_LIB_INFO("[ppm adjust] Read value: %d\n", kv_cfg.ppm_adjust);
    }

    base.header.version = OEM_DATA_VERSION;
    base.header.magic = OEM_DATA_MAGIC;
    base.header.length = sizeof(oem_data_base_data_t);
    base.header.crc = getcrc32(base.data, sizeof(oem_data_base_data_t));

    cli_oem_data_base_save(&base);
}

static void cli_oem_data_set_mac(uint8_t addr[6])
{
    uint8_t status = RET_FAIL;
    uint32_t len = 0;
    uint8_t *data;
    oem_data_base_t base;
    oem_data_base_data_t *oem_data = (oem_data_base_data_t *)&base.data;

    memset((uint8_t *)&base, 0x0, sizeof(oem_data_base_t));

    status = oem_data_base_load(&data, &len);
    if (status == RET_OK) {
        memcpy((uint8_t *)oem_data, data, len);
    }

    memcpy(oem_data->mac, addr, 6);

    base.header.version = OEM_DATA_VERSION;
    base.header.magic = OEM_DATA_MAGIC;
    base.header.length = sizeof(oem_data_base_data_t);
    base.header.crc = getcrc32(base.data, sizeof(oem_data_base_data_t));

    cli_oem_data_base_save(&base);
}

void cli_write_oem_ppm_handler(uint8_t *buffer, uint32_t bufferlen)
{
    uint32_t oem_ppm = buffer[0];

    assert(bufferlen == 1);

    /* set oem ppm */
    cli_oem_data_set_ppm(oem_ppm);
    DBGLOG_LIB_CLI_INFO("cli write oem ppm:%d\n", oem_ppm);

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_OEM_PPM_SET, NULL, 0, 0, RET_OK);
}

void cli_write_oem_mac_handler(uint8_t *buffer, uint32_t bufferlen)
{
    uint8_t *oem_addr = buffer;

    assert(bufferlen == 6);

    /* set oem mac */
    cli_oem_data_set_mac(oem_addr);
    DBGLOG_LIB_CLI_INFO("cli write oem mac: %02X:%02X:%02X:%02X:%02X:%02X\n", buffer[0], buffer[1],
                        buffer[2], buffer[3], buffer[4], buffer[5]);

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_OEM_MAC_SET, NULL, 0, 0, RET_OK);
}

CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_OEM_PPM_SET, cli_write_oem_ppm_handler);
CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_OEM_MAC_SET, cli_write_oem_mac_handler);
