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
#include "os_utils.h"

#include "pmm.h"
#include "cli_audio_definition.h"
#include "cli.h"
#include "cli_audio_spk.h"
#include "cli_command.h"
#include "lib_dbglog.h"

#include "iot_audio_dac.h"

#pragma pack(push) /* save the pack status */
#pragma pack(1)    /* 1 byte align */

typedef struct _cli_audio_spk_dig_gain_param {
    uint8_t spk_chn;
    int16_t gain_step;
    uint8_t up;
} cli_audio_spk_dig_gain_param;

typedef struct _cli_audio_spk_dcdc_1p2_modefy_param {
    uint8_t dcdc_1p2_value;
} cli_audio_spk_dcdc_1p2_modefy_param;

typedef struct _cli_audio_spk_ana_gain_param {
    uint8_t spk_chn;
    int16_t gain_step;
    uint8_t up;
} cli_audio_spk_ana_gain_param;

typedef struct _cli_audio_spk_dig_gain_ack {
    uint16_t result;
    uint64_t reversed;
} cli_audio_spk_dig_gain_ack;

typedef struct _cli_audio_spk_dcdc_1p2_modefy_ack {
    uint16_t result;
    uint64_t reversed;
} cli_audio_spk_dcdc_1p2_modefy_ack;

typedef struct _cli_audio_spk_ana_gain_ack {
    uint16_t result;
    uint64_t reversed;
} cli_audio_spk_ana_gain_ack;
#pragma pack(pop)

void cli_audio_spk_ana_gain_adjust_handler(uint8_t *buffer, uint32_t bufferlen)
{
    (void)bufferlen;
    cli_audio_spk_ana_gain_param *spk_ana_gain_param =
        (cli_audio_spk_ana_gain_param *)buffer;
    cli_audio_spk_ana_gain_ack spk_ana_gain_ack = {0};

    iot_tx_dfe_ana_gain_step_set(spk_ana_gain_param->spk_chn,
                                 spk_ana_gain_param->gain_step);
    iot_tx_dfe_adjust_analog_gain(spk_ana_gain_param->spk_chn,
                                  IOT_TX_DFE_GAIN_CONTROL_DIRECTLY,
                                  spk_ana_gain_param->up);
    os_delay(10);
    cli_interface_msg_response(CLI_MODULEID_AUDIO,
                               CLI_MSGID_SPK_ANA_GAIN_ADJUST,
                               (uint8_t *)&spk_ana_gain_ack,
                               sizeof(cli_audio_spk_ana_gain_ack), 0, RET_OK);
}

void cli_audio_spk_dig_gain_adjust_handler(uint8_t *buffer, uint32_t bufferlen)
{
    (void)bufferlen;
    cli_audio_spk_dig_gain_param *spk_dig_gain_param =
        (cli_audio_spk_dig_gain_param *)buffer;
    cli_audio_spk_dig_gain_ack spk_dig_gain_ack = {0};

    iot_audio_dac_gain_adjust(spk_dig_gain_param->spk_chn,
                              IOT_TX_DFE_GAIN_CONTROL_DIRECTLY,
                              spk_dig_gain_param->up, spk_dig_gain_param->gain_step, true);

    os_delay(10);
    cli_interface_msg_response(CLI_MODULEID_AUDIO,
                               CLI_MSGID_SPK_DIG_GAIN_ADJUST,
                               (uint8_t *)&spk_dig_gain_ack,
                               sizeof(cli_audio_spk_dig_gain_ack), 0, RET_OK);
}

void cli_audio_spk_dcdc_1p2_modefy_handler(uint8_t *buffer, uint32_t bufferlen)
{
    (void)bufferlen;
    cli_audio_spk_dcdc_1p2_modefy_param *spk_dcdc_1p2_param =
        (cli_audio_spk_dcdc_1p2_modefy_param *)buffer;
    cli_audio_spk_dcdc_1p2_modefy_ack spk_dcdc_1p2_ack = {0};

    //1.25v -> 20 << 1 + 1;
    uint8_t value = (spk_dcdc_1p2_param->dcdc_1p2_value << 1) + 1;
    pmm_set_dcdc_1p2(PMM_DCDC_1P2_FW_RAMPUP, 1, &value);

    os_delay(10);
    cli_interface_msg_response(
        CLI_MODULEID_AUDIO, CLI_MSGID_SPK_DCDC_1P2_MODEFY,
        (uint8_t *)&spk_dcdc_1p2_ack, sizeof(cli_audio_spk_dcdc_1p2_modefy_ack),
        0, RET_OK);
}

CLI_ADD_COMMAND(CLI_MODULEID_AUDIO, CLI_MSGID_SPK_DIG_GAIN_ADJUST,
                cli_audio_spk_dig_gain_adjust_handler);

CLI_ADD_COMMAND(CLI_MODULEID_AUDIO, CLI_MSGID_SPK_ANA_GAIN_ADJUST,
                cli_audio_spk_ana_gain_adjust_handler);

CLI_ADD_COMMAND(CLI_MODULEID_AUDIO, CLI_MSGID_SPK_DCDC_1P2_MODEFY,
                cli_audio_spk_dcdc_1p2_modefy_handler);
