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

#ifndef _SRC_LIB_PLAY_CONTROLLER_TONE_PLAY_PLAY_TONE_H_
#define _SRC_LIB_PLAY_CONTROLLER_TONE_PLAY_PLAY_TONE_H_
#ifndef NEW_ARCH

#include "types.h"
#include "modules.h"
#include "player_api.h"

/**
 * @brief audio_player_tone_init.
 *
 * @param[in] asrc_id the asrc id used.
 * @param[in] reset false: init true:factory data reset.
 */
void audio_player_tone_init(uint8_t asrc_id, bool reset);

/**
 * @brief audio_get_tone_asrc_pend_num.
 * @return ture allow send pkt to asrc
           false not allow send pkt to asrc.
 */
bool audio_tone_allow_asrc_pend_pkt(void);

/**
 * @brief audio_player_tone_data_push_2_spk.
 *
 * @param[in] pcm_list the list of data from ring.
 * @param[in] pcm_len the length of pcm_list data.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t audio_player_tone_data_push_2_spk(void *pcm_list, uint16_t pcm_len);

/**
 * @brief  player_set_language
 *
 * @param[in]    current    set tone language.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_set_language(uint8_t current);

/**
 * @brief audio_player_tone_start.
 *
 * @param[in] p_param is the pointer of the tone parameter.
 * @param[in] tone_start_ts specified the sn to start playing tone in mix case.
 *      0 if caller doesn't core about the start ts, use default sn then.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t audio_player_tone_start(player_tone_param_t *p_param, uint32_t tone_start_ts);

/**
 ******************************************************************************
 * @brief audio_player_tone_play_ready: ready to play tone on case that
 *          tone stream start to play and tone speaker can not open successfully,
 *          because last last stream speaker has not been closed.
 *
 * @param[in]   stream_id    last stream that has destroyed, only MUSIC or VOICE.
 ******************************************************************************
 */
void audio_player_tone_play_ready(uint8_t stream_id);

/**
 * @brief  player_tone_stop
 *
 * @param[in]    tone_id    the tone currently playing (find tone data in flash).
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_tone_stop(uint16_t tone_id);

/**
 * @brief  audio_player_tone_stop
 *
 * @param[in]    tone_id    the tone currently playing (find tone data in flash).
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t audio_player_tone_stop(uint16_t tone_id);

/**
 * @brief audio_player_tone_process.
 *
 * @param[in] p_tag         The pointer of datapath data tag.
 * @param[in] sample_num    The sample num of the process.
 * @param[in] underrun      True is underrun case,false is in process.
 */
void audio_player_tone_process(void *p_tag, uint32_t sample_num, bool underrun);

/**
 * @brief audio_player_tone_upsample_set
 *
 * @param[in]   spk_freq    the current speaker freq
 */
void audio_player_tone_upsample_set(uint32_t spk_freq);

/**
 * @brief audio_player_tone_resume
 *      resume tone start,when other stream is ongoing
 *
 * @param[in]   asrc_st    the current asrc state
 *  ASRC_ST_IDLE / ASRC_ST_ONGOING /ASRC_ST_WAIT_STOP /ASRC_ST_WAIT_START
 */
void audio_player_tone_resume(uint8_t asrc_st);

/**
 * @brief audio_player_tone_stream_resume
 *  resume tone start,when other streams are destroyed
 *
 */
void audio_player_tone_stream_resume(uint8_t last_stream);

/**
 * @brief tone_set_stream_vol_coeff
 * @param[in] tone_coeff   config playing tone vol coeff
 * @param[in] other_coeff  config playing music/call vol coeff
 * @return  RET_OK when it's success, else is fail
 *
 */
uint8_t tone_set_stream_vol_coeff(uint8_t tone_coeff, uint8_t other_coeff);
#endif // NEW_ARCH
#endif /* _SRC_LIB_PLAY_CONTROLLER_TONE_PLAY_PLAY_TONE_H_ */
