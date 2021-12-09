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

#ifndef _SRC_LIB_PLAYER_MUSIC_CORES_INC_NPLAYER_MUSIC_ITL_H_
#define _SRC_LIB_PLAYER_MUSIC_CORES_INC_NPLAYER_MUSIC_ITL_H_

#include "types.h"
#include "modules.h"
#include "nplayer_music_api.h"
#include "nplayer_music_core.h"

/**
 * @brief  player_music_create (opt rpc)
 *       malloc some resources to play music.
 *
 * @param[in]    p_param      The pointer of the music parameter.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_music_create(player_music_param_t *p_param);

/**
 * @brief  player_music_destroy (opt rpc)
 *       free some resources to play music.
 *
 * @param[in]    reserve    The reserve param of the music parameter.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_music_destroy(uint32_t reserve);
#endif /* _SRC_LIB_PLAYER_MUSIC_CORES_INC_NPLAYER_MUSIC_ITL_H_ */
