/****************************************************************************

Copyright(c) 2021 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
#ifndef _PLAYER_AUDIO_KV_H
#define _PLAYER_AUDIO_KV_H

#include "storage_controller.h"
typedef enum {
    /* ID for ro_cfg_general_t*/
    VOICE_AEC_ANC_SEND_CFG_KV_ID = AUDIO_PARAM_BASE_ID,  //deprecated
    VOICE_EQ_SEND_CFG_KV_ID,
    VOICE_AGC_SEND_CFG_KV_ID,
    VOICE_DMA_SEND_CFG_KV_ID,
    VOICE_RNN_SEND_CFG_KV_ID,
    VOICE_DMNR_SEND_CFG_KV_ID,
    VOICE_WIND_NOISE_SEND_CFG_KV_ID, //deprecated
    VOICE_EQ_RECV_CFG_KV_ID,
    VOICE_AGC_RECV_CFG_KV_ID,
    VOICE_ANR_SEND_CFG_KV_ID,
    VOICE_AEC_SEND_CFG_KV_ID,
    VOICE_MCRA_SEND_CFG_KV_ID,
    MUSIC_EQ_CFG_KV_ID,
} audio_kv_id_t;

#endif //_PLAYER_AUDIO_KV_H
