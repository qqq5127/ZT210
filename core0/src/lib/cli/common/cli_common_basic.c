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

#include "iot_flash.h"
#include "iot_rom_patch.h"
#include "os_utils.h"
#include "boot_reason.h"
#include "iot_clock.h"
#include "caldata.h"
#include "version.h"
#include "crc.h"
#include "oem.h"
#include "clock.h"
#include "pmm.h"

#include "lib_dbglog.h"
#include "dbglog.h"

#include "cli.h"
#include "cli_common_definition.h"
#include "cli_common_basic.h"
#include "iot_memory_origin.h"
#include "dfs.h"

#pragma pack(push) /* save the pack status */
#pragma pack(1)    /* 1 byte align */
typedef struct cli_chip_info {
    uint32_t sw_ver;
    uint32_t soc_id[2];
    uint8_t reserved[72];
} cli_chip_info_t;

typedef struct cli_log_level {
    uint8_t module;
    uint8_t level;
    uint8_t reserved[2];
} cli_log_level_t;

typedef struct {
    uint16_t ate_ver_major;
    uint16_t ate_ver_minor;
} cli_version_ate_ver_param;
#pragma pack(pop)

#ifdef BUILD_CHIP_WQ7033
extern uint32_t rom_lib_version;
#endif

#define DSP_IROM_LENGTH 0x80000 /* 512k */

typedef struct test_mode_t {
    DFS_OBJ obj;
    CLOCK_CORE clk;
} test_mode_t;

test_mode_t romcrc_test_mode_table[] = {
    {DFS_OBJ_SHARED_FIXED_32_32_32M, CLOCK_CORE_32M},
    {DFS_OBJ_SHARED_FIXED_64_64_64M, CLOCK_CORE_64M},
    {DFS_OBJ_SHARED_FIXED_80_80_160M, CLOCK_CORE_80M},
};

const uint32_t dtop_rom_crc_default = 0xe7447458;
const uint32_t bt_rom_crc_default = 0x13e2b028;
const uint32_t dsp_irom_crc_default = 0xdebb20e3;

void cli_common_basic_get_fw_ver(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

    const char *soft_ver = version_get_global_version();

    DBGLOG_LIB_CLI_INFO("cli get fw version\n");

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_GET_FW_VER, (void *)soft_ver,
                               strlen(soft_ver), 0, (uint8_t)RET_OK);
}

void cli_common_basic_get_ate_ver(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

    cal_data_t *caldata;
    cal_ana_data_cfg_t *cal_cfg;
    cal_data_ana_cfg_t *ana_cfg;
    cli_version_ate_ver_param ate_version_params = {0};
    uint8_t ret = 0;

    DBGLOG_LIB_CLI_INFO("cli get ate version\n");

    /* read calib data from flash */
    caldata = cal_data_load();
    cal_cfg = (cal_ana_data_cfg_t *)caldata->ana_data;
    ana_cfg = (cal_data_ana_cfg_t *)&cal_cfg->ana_cfg;

    uint8_t *pbuf = (uint8_t *)ana_cfg;
    uint32_t size = sizeof(cal_data_ana_cfg_t);
    uint8_t crc8 = (uint8_t)getcrc8(pbuf, size);
    uint8_t pseudo_crc8 = (uint8_t)(getcrc32(pbuf, size) >> 24);
    if (crc8 == cal_cfg->crc || pseudo_crc8 == cal_cfg->crc) {
        ate_version_params.ate_ver_major = ana_cfg->hw_ver_major;
        ate_version_params.ate_ver_minor = ana_cfg->hw_ver_minor;
        ret = RET_OK;
    } else {
        ret = RET_FAIL;
    }

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_GET_ATE_VER,
                               (void *)&ate_version_params, sizeof(ate_version_params), 0,
                               (uint8_t)ret);
}

void cli_common_basic_get_cal_data(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

    cal_data_t *caldata = cal_data_load();

    DBGLOG_LIB_CLI_INFO("cli get cal data\n");

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_GET_CAL_DATA, (void *)caldata,
                               sizeof(cal_data_t), 0, (uint8_t)RET_OK);
}

void cli_common_basic_get_oem_data(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

    oem_data_t *oemdata = oem_data_load();

    DBGLOG_LIB_CLI_INFO("cli get oem data\n");

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_GET_OEM_DATA, (void *)oemdata,
                               sizeof(oem_data_t), 0, (uint8_t)RET_OK);
}

void cli_common_basic_get_anc_data(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

    oem_data_anc_t *ancdata = anc_data_load();
    uint32_t anc_data_len = 0;
    uint8_t ret = RET_OK;
    uint8_t *data = NULL;

    DBGLOG_LIB_CLI_INFO("cli get anc data\n");

    if (ancdata->header.magic == OEM_DATA_MAGIC) {
        uint32_t crc = getcrc32(ancdata->data, ancdata->header.length);

        if (crc == ancdata->header.crc) {
            /* data check pass */
            anc_data_len = ancdata->header.length;
            data = ancdata->data;
            ret = RET_OK;
        } else {
            anc_data_len = 0;
            ret = RET_FAIL;
        }
    }

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_GET_ANC_DATA, (void *)data,
                               anc_data_len, 0, (uint8_t)ret);
}

void cli_common_basic_get_chip_info(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

    cli_chip_info_t chip_info = {0};
    uint32_t soc_id[2] = {0};
    chip_info.sw_ver = 0;
    iot_flash_get_soc_id(soc_id);

    memcpy(chip_info.soc_id, soc_id, sizeof(soc_id));

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_CHIP_INFO_RESP, (uint8_t *)&chip_info,
                               sizeof(chip_info), 0, RET_OK);
}

void cli_common_basic_reset(uint8_t *buffer, uint32_t bufferlen)
{
    assert(bufferlen == 2 || bufferlen == 3);
    uint32_t reset_ok = 0xA5A5A5A5;
    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_SOFT_RST, (uint8_t *)&reset_ok,
                               sizeof(reset_ok), 0, RET_OK);

    uint16_t delay_ms = buffer[0] + (buffer[1] << 8);
    os_delay(delay_ms);
    if (bufferlen == 3) {
        boot_reason_system_reset(buffer[2], 0);
    } else {
        boot_reason_system_reset(BOOT_REASON_SOFT_REASON_SYS, 0xA5);
    }
}

void cli_common_basic_set_clock_mode(uint8_t *buffer, uint32_t bufferlen)
{
    uint8_t ret = RET_FAIL;
    DFS_OBJ new_obj;
    static DFS_OBJ last_dfs_obj = DFS_OBJ_MAX;
    assert(bufferlen == 1);
    new_obj = (DFS_OBJ)buffer[0];
    if (last_dfs_obj != new_obj && new_obj >= DFS_OBJ_SHARED_FIXED_START
        && new_obj < DFS_OBJ_SHARED_FIXED_END) {
        if (last_dfs_obj != DFS_OBJ_MAX) {
            dfs_stop(last_dfs_obj);
        }
        last_dfs_obj = new_obj;
        ret = dfs_start(new_obj, 0);
    }
    DBGLOG_LIB_CLI_INFO("cli_common_basic_set_clock_mode %d, ret %d\n", buffer[0], ret);
    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_SET_CLOCK_MODE, NULL, 0, 0, ret);
}

void cli_common_basic_set_log_level(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(bufferlen);
    cli_log_level_t *log_level = (cli_log_level_t *)buffer;

    DBGLOG_LIB_CLI_INFO("cli set log level module:%d, level:%d\n", log_level->module,
                        log_level->level);
    uint8_t result = dbglog_set_log_level(log_level->module, log_level->level);

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_SET_LOG_LEVEL, NULL, 0, 0, result);
}

void cli_common_basic_get_romlib_ver(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

#ifdef BUILD_CHIP_WQ7033
    uint32_t romlib_version = *(uint32_t *)&rom_lib_version;
    DBGLOG_LIB_CLI_INFO(
        "[ CLI ] cli received get rom version command! rom_lib_address=0x%x rom_lib_version=%x\n",
        &rom_lib_version, romlib_version);

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_GET_ROM_VER,
                               (uint8_t *)&romlib_version, sizeof(romlib_version), 0,
                               (uint8_t)RET_OK);
#endif
}

static uint8_t cli_common_basic_rom_crc_check_unit(CLOCK_CORE clk)
{
    uint32_t dtop_rom_crc;
    uint32_t bt_rom_crc;
    uint32_t dsp_irom_crc;
    uint8_t flag = 0;

    /* dtop */
    dtop_rom_crc = getcrc32((uint8_t *)DCP_ROM_START, DCP_ROM_LENGTH);
    if (dtop_rom_crc == dtop_rom_crc_default) {
        flag |= BIT(0);
    }

    /* bt */
    bt_rom_crc = getcrc32((uint8_t *)BCP_ROM_START, BCP_ROM_LENGTH);
    if (bt_rom_crc == bt_rom_crc_default) {
        flag |= BIT(1);
    }

    /* dsp */
    for (uint32_t i = 0; i < DSP_IROM_LENGTH / sizeof(uint32_t); i++) {
        uint32_t val = *(volatile uint32_t *)(DSP_IROM_START + 4 * i);
        uint32_t first = 1;
        if (first) {
            dsp_irom_crc = getcrc32_update(0xFFFFFFFF, (uint8_t *)&val, 4);
            first = 0;
        } else {
            dsp_irom_crc = getcrc32_update(dsp_irom_crc, (uint8_t *)&val, 4);
        }
    }

    if (dsp_irom_crc == dsp_irom_crc_default) {
        flag |= BIT(2);
    }

    if (dtop_rom_crc != dtop_rom_crc_default || bt_rom_crc != bt_rom_crc_default
        || dsp_irom_crc != dsp_irom_crc_default) {
        DBGLOG_LIB_CLI_INFO(
            "[ROM_CRC_FAIL] CLOCK_CORE_%d dtop[0x%08x] bt[0x%08x] dsp_irom[0x%08x]\n", clk,
            dtop_rom_crc, bt_rom_crc, dsp_irom_crc);
    } else {
        DBGLOG_LIB_CLI_INFO(
            "[ROM_CRC_PASS] CLOCK_CORE_%d dtop[0x%08x] bt[0x%08x] dsp_irom[0x%08x]\n", clk,
            dtop_rom_crc, bt_rom_crc, dsp_irom_crc);
    }

    return flag;
}

static void cli_common_basic_clock_switch(DFS_OBJ dfs_obj, CLOCK_CORE core_clk)
{
    DFS_OBJ new_obj;
    static DFS_OBJ last_dfs_obj = DFS_OBJ_MAX;
    new_obj = dfs_obj;
    if (last_dfs_obj != new_obj && new_obj >= DFS_OBJ_SHARED_FIXED_START
        && new_obj < DFS_OBJ_SHARED_FIXED_END) {
        if (last_dfs_obj != DFS_OBJ_MAX) {
            dfs_stop(last_dfs_obj);
        }
        last_dfs_obj = new_obj;
        dfs_start(new_obj, 0);
    }

    while (clock_get_core_clock(0) != core_clk) {
        os_delay(5);
    }
}

static void cli_common_basic_quality_save(uint32_t quality)
{
    uint8_t status;
    uint8_t *data;
    uint32_t len = 0;
    oem_data_base_t base;
    oem_data_base_data_t *oem_data = (oem_data_base_data_t *)&base.data;

    memset((uint8_t *)&base, 0x0, sizeof(oem_data_base_t));
    status = oem_data_base_load(&data, &len);

    if (status == RET_OK) {
        memcpy((uint8_t *)oem_data, data, len);
    } else {
        DBGLOG_LIB_CLI_INFO("[ROM_CRC_FAIL] Can't find a valid oem!");
    }

    oem_data->quality = quality;
    base.header.version = OEM_DATA_VERSION;
    base.header.magic = OEM_DATA_MAGIC;
    base.header.length = sizeof(oem_data_base_data_t);
    base.header.crc = getcrc32(base.data, sizeof(oem_data_base_data_t));

    iot_flash_write(OEM_DATA_BASE_OFFSET, (uint8_t *)&base, sizeof(oem_data_base_t));
}

void cli_common_basic_rom_crc_check(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

    uint32_t total_flag = 0;

    iot_rom_patch_controller_disable(IOT_BT_ROM_PATCH, IOT_ROM_PATCH0);
    iot_rom_patch_controller_disable(IOT_BT_ROM_PATCH, IOT_ROM_PATCH1);

    cli_common_basic_clock_switch(DFS_OBJ_SHARED_FIXED_16_16_16M, CLOCK_CORE_16M);
    int8_t tmp = pmm_use_compatible_voltage(-2);

    for (uint8_t i = 0; i < ARRAY_SIZE(romcrc_test_mode_table); i++) {

        cli_common_basic_clock_switch(romcrc_test_mode_table[i].obj, romcrc_test_mode_table[i].clk);

        uint8_t flag = cli_common_basic_rom_crc_check_unit(romcrc_test_mode_table[i].clk);

        total_flag |= ((flag & 0x7) << (i * 3));
    }

    cli_common_basic_clock_switch(DFS_OBJ_SHARED_FIXED_16_16_16M, CLOCK_CORE_16M);
    pmm_use_compatible_voltage(tmp);

    iot_rom_patch_controller_enable(IOT_BT_ROM_PATCH, IOT_ROM_PATCH0);
    iot_rom_patch_controller_enable(IOT_BT_ROM_PATCH, IOT_ROM_PATCH1);

    cli_common_basic_quality_save(total_flag);

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_ROM_CRC_CHECK, (uint8_t *)&total_flag,
                               sizeof(uint32_t), 0, RET_OK);
}

CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_GET_FW_VER, cli_common_basic_get_fw_ver);
CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_GET_ATE_VER, cli_common_basic_get_ate_ver);
CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_GET_CAL_DATA, cli_common_basic_get_cal_data);
CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_GET_OEM_DATA, cli_common_basic_get_oem_data);
CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_GET_ANC_DATA, cli_common_basic_get_anc_data);
CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_GET_CHIP_INFO, cli_common_basic_get_chip_info);
CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_SOFT_RST, cli_common_basic_reset);
CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_SET_CLOCK_MODE, cli_common_basic_set_clock_mode);
CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_SET_LOG_LEVEL, cli_common_basic_set_log_level);
CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_GET_ROM_VER, cli_common_basic_get_romlib_ver);
CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_ROM_CRC_CHECK, cli_common_basic_rom_crc_check);
