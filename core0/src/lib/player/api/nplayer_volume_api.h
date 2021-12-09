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

#ifndef _SRC_LIB_PLAYER_API_NPLAYER_VOLUME_API_H_
#define _SRC_LIB_PLAYER_API_NPLAYER_VOLUME_API_H_

#include "types.h"
#include "modules.h"
#include "nplayer_api.h"

#define PLAYER_VOLUME_MUTE_GAIN -90   //db
#ifdef NEW_ARCH

/**
 * @brief This function is to adjust the volume of the palyer.
 *
 * @param id The player stream id of the player volume. @see stream_id_t
 *           STREAM_MUSIC / STREAM_VOICE / STREAM_TONE /
 * @param db is the decibels of the player volume.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_volume_set(uint8_t id, int8_t db);

/**
 * @brief  player_volume_get
 *      get the volume of the current stream.
 *
 * @param[in] id The player stream id of the player volume. @see stream_id_t
 *           STREAM_MUSIC / STREAM_VOICE / STREAM_TONE /
 *
 * @return  The current stream volume.
 */
int8_t player_volume_get(uint8_t id);

/**
 * @brief This function is to mute speaker voice.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_volume_mute(void);

/**
 * @brief This function is to unmute speaker voice.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_volume_unmute(void);
#endif
#endif /* _SRC_LIB_PLAYER_API_NPLAYER_VOLUME_API_H_ */
