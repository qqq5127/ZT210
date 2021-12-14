
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

/*
 * INCLUDE FILES
 ****************************************************************************
 */
#include "types.h"
#include "string.h"

#include "lib_dbglog.h"
#include "os_mem.h"
#include "audio_anc.h"
#include "player_audio_config_define.h"
#include "storage_controller.h"
#include "player_audio_kv_define.h"
#include "player_internal.h"
#include "cfg_dsp.h"

#define BIT_SET(data, bit) ((data) |= (0x01 << (bit)))
#define BIT_CLR(data, bit) ((data) &= (~(0x01 << (bit))))
#define SET_OR_CLR_BIT(data, bit, state) (state ? BIT_SET(data, bit) : BIT_CLR(data, bit))

static uint32_t voice_enable_state = 0;

static uint8_t dtop_cfg_per_voice_param(uint8_t *per_cfg, uint32_t per_cfg_len, uint32_t kv_idx)
{
    uint8_t ret;
    uint32_t len = per_cfg_len;

    ret = (uint8_t)storage_read(AUDIO_PARAM_BASE_ID, kv_idx, (void *)(per_cfg), &len);

    if (ret || len < per_cfg_len) {
        memset(per_cfg, 0, per_cfg_len);
    }

    return ret;
}

static void dtop_cfg_all_voice_param(void)
{
    voice_cfg_all_t *voice_cfg = (voice_cfg_all_t *)os_mem_malloc(PLAYER_MID, sizeof(voice_cfg_all_t));
    assert(voice_cfg != NULL);
    if (voice_cfg == NULL) {        //lint !e774 not always false if assert is empty
        return;
    }

    dtop_cfg_per_voice_param((uint8_t *)&voice_cfg->voice_dma_send_cfg,  sizeof(dma_send_cfg_t),  VOICE_DMA_SEND_CFG_KV_ID);
    SET_OR_CLR_BIT(voice_enable_state, VOICE_PARAM_DMA_SEND, voice_cfg->voice_dma_send_cfg.enable_dma);
    dtop_cfg_per_voice_param((uint8_t *)&voice_cfg->voice_dmnr_send_cfg, sizeof(dmnr_send_cfg_t), VOICE_DMNR_SEND_CFG_KV_ID);
    SET_OR_CLR_BIT(voice_enable_state, VOICE_PARAM_DMNR_SEND, voice_cfg->voice_dmnr_send_cfg.enable_dmnr);
    dtop_cfg_per_voice_param((uint8_t *)&voice_cfg->voice_mcra_send_cfg, sizeof(mcra_send_cfg_t), VOICE_MCRA_SEND_CFG_KV_ID);
    SET_OR_CLR_BIT(voice_enable_state, VOICE_PARAM_MCRA_SEND, voice_cfg->voice_mcra_send_cfg.enable_mcra);
    dtop_cfg_per_voice_param((uint8_t *)&voice_cfg->voice_aec_send_cfg,  sizeof(aec_send_cfg_t),  VOICE_AEC_SEND_CFG_KV_ID);
    SET_OR_CLR_BIT(voice_enable_state, VOICE_PARAM_AEC_SEND, voice_cfg->voice_aec_send_cfg.enable_aec);
    dtop_cfg_per_voice_param((uint8_t *)&voice_cfg->voice_anr_send_cfg,  sizeof(anr_send_cfg_t),  VOICE_ANR_SEND_CFG_KV_ID);
    SET_OR_CLR_BIT(voice_enable_state, VOICE_PARAM_ANR_SEND, voice_cfg->voice_anr_send_cfg.enable_anr);
    dtop_cfg_per_voice_param((uint8_t *)&voice_cfg->voice_rnn_send_cfg,  sizeof(rnn_send_cfg_t),  VOICE_RNN_SEND_CFG_KV_ID);
    SET_OR_CLR_BIT(voice_enable_state, VOICE_PARAM_RNN_SEND, voice_cfg->voice_rnn_send_cfg.enable_rnn);
    dtop_cfg_per_voice_param((uint8_t *)&voice_cfg->voice_agc_send_cfg,  sizeof(agc_cfg_t),       VOICE_AGC_SEND_CFG_KV_ID);
    SET_OR_CLR_BIT(voice_enable_state, VOICE_PARAM_AGC_SEND, voice_cfg->voice_agc_send_cfg.enable_agc);
    dtop_cfg_per_voice_param((uint8_t *)&voice_cfg->voice_eq_send_cfg,   sizeof(voice_eq_cfg_t),  VOICE_EQ_SEND_CFG_KV_ID);
    SET_OR_CLR_BIT(voice_enable_state, VOICE_PARAM_EQ_SEND, voice_cfg->voice_eq_send_cfg.voice_eq_cfg.enable_eq_filter);
    dtop_cfg_per_voice_param((uint8_t *)&voice_cfg->voice_agc_recv_cfg,  sizeof(agc_cfg_t),       VOICE_AGC_RECV_CFG_KV_ID);
    SET_OR_CLR_BIT(voice_enable_state, VOICE_PARAM_AGC_RECV, voice_cfg->voice_agc_recv_cfg.enable_agc);
    dtop_cfg_per_voice_param((uint8_t *)&voice_cfg->voice_eq_recv_cfg,   sizeof(voice_eq_cfg_t),  VOICE_EQ_RECV_CFG_KV_ID);
    SET_OR_CLR_BIT(voice_enable_state, VOICE_PARAM_EQ_RECV, voice_cfg->voice_eq_recv_cfg.voice_eq_cfg.enable_eq_filter);
    dtop_cfg_per_voice_param((uint8_t *)&voice_cfg->voice_dmnr_send_cfg2,   sizeof(dmnr_send_cfg2_t),   VOICE_DMNR_SEND_CFG2_KV_ID);
    SET_OR_CLR_BIT(voice_enable_state, VOICE_PARAM_DMNR2_SEND, voice_cfg->voice_dmnr_send_cfg2.enable_dmnr_cfg2);
    dtop_cfg_per_voice_param((uint8_t *)&voice_cfg->voice_ns_recv_cfg,   sizeof(ns_recv_cfg_t),  VOICE_NS_RECV_CFG_KV_ID);
    SET_OR_CLR_BIT(voice_enable_state, VOICE_PARAM_NS_RECV, voice_cfg->voice_ns_recv_cfg.enable_ns_recv);

    dsp_config_voice_param(VOICE_PARAM_INIT, (void *)voice_cfg, sizeof(voice_cfg_all_t));

    os_mem_free(voice_cfg);
}

uint32_t cfg_dsp_entry(void)
{
    dtop_cfg_all_voice_param();

    return RET_OK;
}

uint32_t get_dsp_voice_enable_state(void)
{
    return voice_enable_state;
}
