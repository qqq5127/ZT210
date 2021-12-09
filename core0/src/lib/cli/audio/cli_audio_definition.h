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

#ifndef LIB_CLI_AUDIO_DEFINITION_H
#define LIB_CLI_AUDIO_DEFINITION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    // anc commands
    CLI_MSGID_ANC_SET_COEFF_PARAM,
    CLI_MSGID_ANC_SET_COEFF_PARAM_ACK,

    CLI_MSGID_SPK_DIG_GAIN_ADJUST,
    CLI_MSGID_SPK_ANA_GAIN_ADJUST,
    CLI_MSGID_SPK_DCDC_1P2_MODEFY,

    CLI_MSGID_ANC_DUMP_START,
    CLI_MSGID_ANC_DUMP_STOP,
    CLI_MSGID_ANC_PLAY_SWEEP_START,
    CLI_MSGID_ANC_PLAY_SWEEP_STOP,
    CLI_MSGID_ANC_WIRITE_COEFF_TO_FLASH,
    CLI_MSGID_ANC_CHECK_COEFF_FROM_FLASH,
    CLI_MSGID_ANC_PORT_OPEN,
    CLI_MSGID_ANC_PORT_CLOSE,
    CLI_MSGID_ANC_PLAY_SWEEP_INIT,
    CLI_MSGID_ANC_SECRET_HANDSHAKE,
    CLI_MSGID_ANC_DUMP_INIT,
    CLI_MSGID_ANC_READ_COEFF_FROM_FLASH,
    CLI_MSGID_SWEEP_SINE_TONE_START,
    CLI_MSGID_SWEEP_SINE_TONE_STOP,
    CLI_MSGID_SWEEP_TDD_NOISE_DUMP_START,
    CLI_MSGID_AUDIO_DUMP_MODE_SET,
    CLI_MSGID_AUDIO_LOOPBACK_START,
    CLI_MSGID_AUDIO_LOOPBACK_STOP,
    CLI_MSGID_SPP_AUDIO_DUMP_PARAM_SET,
    CLI_MSGID_ANC_MAX_NUM = 199,

    // voice commands
    CLI_MSGID_VOICE_PARAM_GET_REQ           = 200,
    CLI_MSGID_VOICE_PARAM_GET_RESP          = 201,
    CLI_MSGID_VOICE_PARAM_TRY_REQ           = 202,
    CLI_MSGID_VOICE_PARAM_TRY_RESP          = 203,
    CLI_MSGID_VOICE_PARAM_COMMIT_REQ        = 204,
    CLI_MSGID_VOICE_PARAM_COMMIT_RESP       = 205,

    // music commands
    CLI_MSGID_MUSIC_PARAM_GET_REQ           = 206,
    CLI_MSGID_MUSIC_PARAM_GET_RESP          = 207,
    CLI_MSGID_MUSIC_PARAM_TRY_REQ           = 208,
    CLI_MSGID_MUSIC_PARAM_TRY_RESP          = 209,
    CLI_MSGID_MUSIC_PARAM_COMMIT_REQ        = 210,
    CLI_MSGID_MUSIC_PARAM_COMMIT_RESP       = 211,

    CLI_MSGID_VOICE_PARAM_ENABLE_CTRL_SET   = 212,
    CLI_MSGID_VOICE_PARAM_ENABLE_CTRL_GET   = 213,//TODO: the enable status is not yet acquired from dsp
    CLI_MSGID_EQ_PARAM_BYPASS_EQ_SET        = 214,
    CLI_MSGID_SPK_MIC_DUMP_START            = 215,
    CLI_MSGID_EQ_DRC_PARAM_SET              = 216,
    CLI_MSGID_SPK_STREAM_GAIN_SET           = 217,
    CLI_MSGID_ANC_SWITCH_CALLBACK_BYPASS_SET= 218,
    CLI_MSGID_AUDIO_HOWLROUND_CFG_SET       = 219,
    CLI_MSGID_VOICE_MUSIC_CFG_MAX           = 249,

    //audio production test
    CLI_MSGID_ENC_MIC_SWITCH                = 250,
    CLI_MSGID_EQ_FOR_SPK_CAL_GET            = 251,
    CLI_MSGID_EQ_FOR_SPK_CAL_COMMIT         = 252,
    CLI_MSGID_AUDIO_PATH_DEF_CHAN_GET       = 253,
    CLI_MSGID_AUDIO_PRODUCTION_TEST_MAX     = 299,

    //vad
    CLI_MSGID_VAD_START                     = 300,
    CLI_MSGID_VAD_STOP                      = 301,
    CLI_MSGID_VAD_MAX                       = 309,

    CLI_MSGID_AUDIO_MAX_NUM,
} CLI_AUDIO_MSGID;

#ifdef __cplusplus
}
#endif

#endif /* LIB_CLI_AUDIO_DEFINITION_H */
