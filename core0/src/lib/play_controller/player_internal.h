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
#ifndef PLAYER_INTERNAL__H_
#define PLAYER_INTERNAL__H_

#ifdef __cplusplus
extern "C" {
#endif
#include "player_api.h"

/*
 * DEFINES
 ****************************************************************************
 */
#define BTCORE_STREAM_RESUME_ENABLE
//dsp sync sys info type bt role
#define DSP_SYNC_INFO_TYPE_BT_ROLE 0

enum {
    DSP_INFO_TWS_ROLE_TYPE = 0,
    DSP_INFO_ANC_STATE_TYPE = 1,
    DSP_INFO_SPK_GENTLY_TYPE = 2,
};

enum {
    // player speaker hw jitter(asrc fifo) underrun error.
    PLAYER_SPK_JT_UNDERRUN = 0,
    // player speaker hw jitter(asrc fifo) watermark is too low,we should send fast.
    PLAYER_SPK_JT_LOW,
    // player speaker hw jitter watermark(asrc fifo) is too high,we should send slow.
    PLAYER_SPK_JT_HIGH,
};

typedef void (*asrc_play_handle_cb)(uint32_t reason, uint32_t sn);
typedef void (*btcore_stream_resume_cb)(uint8_t stream_id);
typedef void (*player_restart_req_cb)(uint32_t reason, uint32_t reserve);
typedef uint8_t (*player_delay_mode_set_cb)(player_delay_mode_t mode);
typedef uint8_t (*player_delay_mode_config_cb)(player_delay_mode_param_t *p_param);

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************
 */

/**
 * @brief handler asrc play reason.
 *
 * @param[in] reason sarc play event noteci.
 * @param[in] sn the sequence number of pcm sample.
 */
void asrc_play_error_handler(uint32_t reason, uint32_t sn);

/**
 * @brief asrc play handle register.
 *
 * @param[in] cb register play handle.
 */
void asrc_play_handle_register(asrc_play_handle_cb cb);

/**
 * @brief resume music or voice stream handler.
 *
 * @param[in] stream_id MUSIC or VOICE.
 */
void btcore_stream_resume(uint32_t stream_id);

/**
 * @brief register the handle that resume music or voice stream.
 *
 * @param[in] cb resume music or voice stream callback.
 */
void btcore_stream_resume_handler_register(btcore_stream_resume_cb cb);

/**
 * @brief player_notice_user_event.
 *
 * @param[in] event which player event.
 * @param[in] param notice user this event param.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_notice_user_event(uint8_t event, player_evt_param_t *param);

/**
 ******************************************************************************
 * @brief audio_sys_create_stream
 *
 * @param[in] stream_id    The create stream id.
 *      SM_MUSIC / SM_VOICE / SM_TONE
 * @return uint8_t RET_OK for success else error.
 ******************************************************************************
 */
uint8_t audio_sys_create_stream(uint8_t stream_id);

/**
 ******************************************************************************
 * @brief audio_sys_destory_stream
 *
 * @param[in] stream_id    The create stream id.
 *      SM_MUSIC / SM_VOICE / SM_TONE
 * @return uint8_t RET_OK for success else error.
 ******************************************************************************
 */
uint8_t audio_sys_destory_stream(uint8_t stream_id);

/**
 ******************************************************************************
 * @brief audio_sys_stream_get
 *
 * @return The stream mask state.
 ******************************************************************************
 */
uint8_t audio_sys_stream_get(void);

/**
 ******************************************************************************
 * @brief audio_sys_resume_stream
 *      resume pending stream
 * @return uint8_t RET_OK for success else error.
 ******************************************************************************
 */
uint8_t audio_sys_resume_stream(void);

/**
 * @brief audio_sys_speaker_config
 *
 * @param[in] stream_id    The create stream id.
 *      SM_MUSIC / SM_VOICE / SM_TONE
 * @param[in] open    True is open, false is close.
 * @param[in] freq    The freq of the stream.
 * @param[in] ppm is the num of asrc_ppm_unit.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t audio_sys_speaker_config(uint8_t stream_id, uint8_t open, uint32_t freq, int32_t ppm);

/**
 * @brief This function is to set sample count.
 *
 * @param cnt is the sample count.
 */
void player_set_sample_cnt(uint32_t cnt);

/**
 * @brief This function is to set voice parameter to DSP.
 *
 * @param[in] param_type    the type if the parameter.
 * @param[in] cfg   buffer of the cfg data.
 * @param[in] size  the size of the cfg data.
 */
uint32_t dsp_config_voice_param(uint32_t param_type, void *cfg, uint32_t size);

/**
 * @brief This function is to turn on or off voice module in DSP.
 *
 * @param[in] module_switch   each bit means if the corresponding voice module need to be
 *                  enabled or not (0: disable. 1: enable). please check enum voice_param_type_t
 *                  to find out the detail bit stands for which module.
 */
uint32_t dsp_config_voice_module_switch(uint32_t module_switch);

/**
 * @brief This function is to turn notice DSP mix tone vol.
 * @param[in] music_vol    current music vol(db).
 * @param[in] call_vol     current call vol(db).
 * @param[in] tone_vol     current tone vol(db).
 * @param[in] tone_a       mix tone scale value
 * @param[in] other_b      mix music/call scale value,
 *                         tone_a + other_b = 128.
 * @param[in] reserved     for feture use.
 *
 */
uint32_t dsp_set_vol(uint32_t music_vol, uint32_t call_vol,
                    uint32_t tone_vol, uint32_t tone_a,
                    uint32_t other_b, uint32_t reserved);

/**
 * @brief This function is sync sys info between dsp with bt.
 * @param[in] type         the type of sys info.
 * @param[in] val          the value of the sys info type.
 * @param[in] reserve1     for feture use
 * @param[in] reserve2     for feture use
 *
 */
uint32_t dsp_sync_sys_info(uint32_t type, uint32_t val, uint32_t reserve1, uint32_t reserve2);

/**
 * @brief btcore_player_restart_req.
 *
 * @param[in] reason       The restart req reason.
 * @param[in] reserve      The reserve param for feture use.
 */
void btcore_player_restart_req(uint32_t reason, uint32_t reserve);

/**
 * @brief btcore_player_restart_req_register.
 *
 * @param[in] cb register bt core play restart req handler.
 */
void btcore_player_restart_req_register(player_restart_req_cb cb);

/**
 * @brief player_delay_mode_set_hdl_register.
 *
 * @param[in] cb register bt core player delay mode set handler.
 */
void player_delay_mode_set_hdl_register(player_delay_mode_set_cb cb);

/**
 * @brief player_delay_mode_config_hdl_register.
 *
 * @param[in] cb register bt core player delay mode config handler.
 */
void player_delay_mode_config_hdl_register(player_delay_mode_config_cb cb);

#ifdef __cplusplus
}
#endif

#endif /* PLAYER_INTERNAL__H_ */
