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

#ifndef _SRC_LIB_PLAYER_TONE_CORES_NPLAYER_TONE_CORE_H_
#define _SRC_LIB_PLAYER_TONE_CORES_NPLAYER_TONE_CORE_H_

/*
 * INCLUDE FILES
 ****************************************************************************
 */
#include "types.h"
#include "nplayer_task_core.h"
#ifdef NEW_ARCH

/// tone resume stage
typedef enum {
    /// tone resume stage that init context
    TONE_RESUME_STAGE_CTX_INIT = 0,
    /// tone resume stage that open tone speaker
    TONE_RESUME_STAGE_SPK_OPEN,
} tone_resume_t;

/**
 * @brief player_tone_process.
 *
 * @param[in] p_tag         The pointer of datapath data tag.
 * @param[in] sample_num    The sample num of the process.
 */
void player_tone_process(void *p_tag, uint32_t sample_num);

/**
 * @brief player_tone_err_hdl
 *
 * @param[in] p_msg      The param of error
 * @param[in] p_param    The p_param of mic/spker.
 */
void player_tone_err_hdl(void *p_msg, void *p_param);

/**
 * @brief send msg about playing tone mixed
 *        when the first dma callback is coming.
 */
void player_tone_mix_msg_send(void);

/**
 * @brief player_tone_check_playing
 *      check whether tone is on going
 */
bool player_tone_check_playing(void);

/**
 * @brief audio_player_tone_upsample_set
 *
 * @param[in]   spk_freq    the current speaker freq
 */
void player_tone_upsample_set(uint32_t spk_freq);

/**
 * @brief This function is to register the event callback handler.
 * @param cb is the user event callback.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_tone_register_event_callback(void *cb);

/**
 * @brief player_tone_timeout_cb.
 *
 * @param[in]   cb        close speaker callback.
 * @param[in]   stage     tone resume stage.
 *
 */
void player_tone_resume_msg_send(player_close_spk_cb cb, uint8_t stage);

#endif

// the followings is rpc declarations
/**
 * @brief dsp stop play tone.
 *
 * @param[in] reserved future use.
 * @return uint8_t RET_OK for success else error.
 */
uint32_t dsp_stop_tone(uint32_t reserved);

/**
 * @brief dsp play tone start.
 *
 * @param[in] addr tone address.
 * @param[in] len tone len.
 * @param[in] alg tone codec.
 * @param[in] sample_rate tone sample_num.
 * @param[in] ts_start the time the tone play.
 * @param[in] val tone tone play voice.
 * @return uint8_t RET_OK for success else error.
 */
uint32_t dsp_start_tone(uint32_t addr, uint32_t len, uint32_t alg, uint32_t sample_rate,
                        uint32_t ts_start, uint32_t val);

/**
 * @brief player_tone_set_stream_vol_coeff
 * @param[in] tone_coeff   config playing tone vol coeff
 * @param[in] other_coeff  config playing music/call vol coeff
 * @return  RET_OK when it's success, else is fail
 *
 */
uint8_t tone_set_stream_vol_coeff(uint8_t tone_coeff, uint8_t other_coeff);
#endif /* _SRC_LIB_PLAYER_TONE_CORES_NPLAYER_TONE_CORE_H_ */
