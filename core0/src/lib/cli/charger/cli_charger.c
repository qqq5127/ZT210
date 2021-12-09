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

#include "lib_dbglog.h"
#include "dbglog.h"
#include "caldata.h"

#include "iot_charger.h"

#include "pmm.h"

#include "cli.h"
#include "cli_charger_definition.h"
#include "cli_charger.h"

#pragma pack(push) /* save the pack status */
#pragma pack(1)    /* 1 byte align */
typedef struct cli_charger_current {
    uint16_t cur_ma;
    uint16_t reserved;
} cli_charger_current_t;
#pragma pack(pop)


void cli_charger_disable_gpio_mode(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

    iot_charger_gpio_enable(false);

    cli_interface_msg_response(CLI_MODULEID_DRIVER_CHARGER, CLI_MSGID_DISABLE_CHARGER_PIN_GPIO_MODE,
                               NULL, 0, 0, RET_OK);
}

void cli_charger_set_current(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(bufferlen);
    cli_charger_current_t *cfg = (cli_charger_current_t *)buffer;

    if (bufferlen != sizeof(cli_charger_current_t)) {
        cli_interface_msg_response(CLI_MODULEID_DRIVER_CHARGER,
                                   CLI_MSGID_DISABLE_CHARGER_SET_CURRENT, NULL, 0, 0, RET_INVAL);
    }

    DBGLOG_LIB_CLI_INFO("[CLI][CHARGER] set current to %dmA\n", cfg->cur_ma);
    iot_charger_set_current(cfg->cur_ma * 10);

    cli_interface_msg_response(CLI_MODULEID_DRIVER_CHARGER, CLI_MSGID_DISABLE_CHARGER_SET_CURRENT,
                               NULL, 0, 0, RET_OK);
}

void cli_charger_get_config(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

    uint32_t value[4];
    pmm_get_pmm_ldo_config(value);

    uint8_t stage = ((value[0] >> 3) & 0x7) + 1;
    for (uint8_t i = 0; i < stage; i++) {
        DBGLOG_LIB_CLI_INFO("[CLI][CHARGER] config stage %d: 0x%08x\n", i, value[3 - i]);
    }

    uint8_t seg = 0;
    uint8_t code = 0;
    switch (stage) {
        case 1:
            code = (value[3] >> 11) & (BIT(6) - 1);
            seg = (value[3] >> 17) & (BIT(2) - 1);
            break;
        case 2:
            code = (value[2] >> 11) & (BIT(6) - 1);
            seg = (value[2] >> 17) & (BIT(2) - 1);
            break;
        case 3:
            code = (value[1] >> 11) & (BIT(6) - 1);
            seg = (value[1] >> 17) & (BIT(2) - 1);
            break;
        default:
            break;
    }

    DBGLOG_LIB_CLI_INFO("[CLI][CHARGER] config seg %d code %d\n", seg, code);

    uint8_t pmos_code = (uint8_t)cal_data_trim_code_get(CHG_IOUT_STEP_TRIM_CODE);
    uint8_t res_code = (uint8_t)cal_data_trim_code_get(CHG_IOUT_RANGE_TRIM_CODE);
    DBGLOG_LIB_CLI_INFO("[CLI][CHARGER] cal pmos %d res %d\n", pmos_code, res_code);

    cal_data_charger charger_cal_data;
    cal_data_charger_get(&charger_cal_data);

    DBGLOG_LIB_CLI_INFO("[CLI][CHARGER] chg iout range 0 min %f max %f\n",
                        *(uint32_t *)&charger_cal_data.chg_iout_range[0][0],
                        *(uint32_t *)&charger_cal_data.chg_iout_range[0][1]);
    DBGLOG_LIB_CLI_INFO("[CLI][CHARGER] chg iout range 1 min %f max %f\n",
                        *(uint32_t *)&charger_cal_data.chg_iout_range[1][0],
                        *(uint32_t *)&charger_cal_data.chg_iout_range[1][1]);
    DBGLOG_LIB_CLI_INFO("[CLI][CHARGER] chg iout range 2 min %f max %f\n",
                        *(uint32_t *)&charger_cal_data.chg_iout_range[2][0],
                        *(uint32_t *)&charger_cal_data.chg_iout_range[2][1]);
    DBGLOG_LIB_CLI_INFO("[CLI][CHARGER] chg iout range 3 min %f max %f\n",
                        *(uint32_t *)&charger_cal_data.chg_iout_range[3][0],
                        *(uint32_t *)&charger_cal_data.chg_iout_range[3][1]);

    cli_interface_msg_response(CLI_MODULEID_DRIVER_CHARGER, CLI_MSGID_DISABLE_CHARGER_GET_CONFIG,
                               NULL, 0, 0, RET_OK);
}

CLI_ADD_COMMAND(CLI_MODULEID_DRIVER_CHARGER, CLI_MSGID_DISABLE_CHARGER_PIN_GPIO_MODE,
                cli_charger_disable_gpio_mode);

CLI_ADD_COMMAND(CLI_MODULEID_DRIVER_CHARGER, CLI_MSGID_DISABLE_CHARGER_SET_CURRENT,
                cli_charger_set_current);

CLI_ADD_COMMAND(CLI_MODULEID_DRIVER_CHARGER, CLI_MSGID_DISABLE_CHARGER_GET_CONFIG,
                cli_charger_get_config);
