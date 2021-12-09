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

#ifndef _SRC_LIB_PLAYER_API_NPLAYER_TONE_API_H_
#define _SRC_LIB_PLAYER_API_NPLAYER_TONE_API_H_

#include "types.h"
#include "modules.h"
#include "nplayer_api.h"

/* min time in ms to start HW timer before play sync tone. */
#ifndef PLAY_TONE_MIN_RESERVED_MS
#define PLAY_TONE_MIN_RESERVED_MS 30
#endif

/* max time in ms to start HW timer before play sync tone.
 * tone module won't start HW timer with duration larger than this value.
 */
#ifndef PLAY_TONE_MAX_RESERVED_US
#define PLAY_TONE_MAX_RESERVED_US (600*1000)
#endif

/// player tone vol mode
typedef enum {
    //pc tool config each tone vol
    VOL_MD_INDEPENDENT,
    //tone vol follow a2dp/sco
    VOL_MD_FOLLOW,
    //custom decide tone vol
    VOL_MD_CUSTOM,
} player_tone_vol_md_t;

///tone parameters
typedef struct _player_tone_param_t {
    //tone id
    uint16_t tone_id;
    //speaker out channel @see player_spk_out_t
    uint8_t spk;
    //@see player_tone_vol_md_t
    uint8_t vol_md;
    //volume unit db
    int8_t vol;
} player_tone_param_t;

///language info
typedef struct _player_language_info_t {
    //current language id
    uint8_t current;
    // total support language num
    uint8_t total;
} player_language_info_t;
#ifdef NEW_ARCH
/**
 * @brief player_tone_init.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_tone_init(void);

/**
 * @brief  audio_player_tone_start
 *       start play tone.
 *
 * @param[in]    p_param        The pointer of the tone parameter.
 * @param[in]    tone_start_ts  The timestamp that the tone start playing.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t audio_player_tone_start(player_tone_param_t *p_param, uint32_t tone_start_ts);

/**
 * @brief  player_tone_start
 *       start play tone.
 *
 * @param[in]    p_param      The pointer of the tone parameter.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_tone_start(player_tone_param_t *p_param);

/**
 * @brief  player_tone_stop
 *       stop play tone.
 *
 * @param[in]    tone_id    the tone currently playing (find tone data in flash).
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_tone_stop(uint16_t tone_id);

/**
 * @brief  player_get_language_info
 *       get current language index and total support language num info.
 *
 * @param[out]    p_info    The pointer of the language info.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_get_language_info(player_language_info_t *p_info);

/**
 * @brief  player_set_language
 *       set current use language index (0~total support).
 *
 * @param[in]    current    The current used language.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_set_language(uint8_t current);

/**
 * @brief player_get_tone_ts
 *   get tone start timestamp to mix with music / voice
 *
 * @return the timestamp tone should to start
 */
uint32_t player_get_tone_start_ts(uint32_t delay_ms);

/**
 * @brief  player_tone_create_stream
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @ref RET_TYPE .
 */
uint8_t player_tone_create_stream(void);

/**
 * @brief  player_tone_stream_exist
 *
 * @return 1 if tone stream exist, 0 if not exist.
 */
uint8_t player_tone_stream_exist(void);

/**
 * @brief  player_tone_destroy_stream
 *
 * @return  RET_OK when it's success.
 */
uint8_t player_tone_destroy_stream(void);

#endif // NEW_ARCH
#endif /* _SRC_LIB_PLAYER_API_NPLAYER_TONE_API_H_ */
