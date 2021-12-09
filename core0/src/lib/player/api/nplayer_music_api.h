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

#ifndef _SRC_LIB_PLAYER_API_NPLAYER_MUSIC_API_H_
#define _SRC_LIB_PLAYER_API_NPLAYER_MUSIC_API_H_
#include "types.h"
#include "modules.h"
#include "nplayer_api.h"

typedef struct _player_music_param_t {
    //codec info
    player_codec_t codec;
    //volume unit db
    int8_t vol;
} player_music_param_t;
#ifdef NEW_ARCH

/**
 * @brief player_music_init.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_music_init(void);

/**
 * @brief  player_music_start
 *       start play music.
 *
 * @param[in]    p_param      The pointer of the music parameter.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_music_start(player_music_param_t *p_param);

/**
 * @brief This function is to config music.
 *
 * @param codec_type is the type of the music.
 * @param p_cfg is the music codec info.
 */
void player_music_codec_cfg(uint8_t codec_type, void *p_cfg);

/**
 * @brief  player_music_stop
 *       stop play music.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_music_stop(void);

/**
 * @brief This function is to receive music data.
 *
 * @param p_data is the pointer of the music data.
 * @param len is the length of the music data.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_music_in(void *p_data, uint32_t len);
#endif // NEW_ARCH
#endif /* _SRC_LIB_PLAYER_API_NPLAYER_MUSIC_API_H_ */
