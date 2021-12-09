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

#ifndef _SRC_LIB_PLAYER_H_
#define _SRC_LIB_PLAYER_H_

#ifndef NEW_ARCH
#include "types.h"
#include "modules.h"

/**
 * @brief player_vol_init
 *      player volume env init
 * @param[in] tone_vol_mode set player tone mode.
 */
void player_vol_init(uint8_t tone_vol_mode);

/**
 * @brief player_adj_vol_msg_hdl
 *
 * @param[in] p_msg_data    The message of player adj vol.
 */
void player_adj_vol_msg_hdl(void *p_msg_data);

/**
 * @brief  player_vol_resume
 *      resume the volume of the current stream.
 *
 * @param[in] id The player stream id of the player volume. @see stream_id_t
 *           STREAM_MUSIC / STREAM_VOICE / STREAM_TONE /
 */
void player_vol_resume(uint8_t id);

/**
 * @brief  player_current_vol_get
 *      get the volume of the current stream.
 *
 * @param[in] id The player stream id of the player volume. @see stream_id_t
 *           STREAM_MUSIC / STREAM_VOICE / STREAM_TONE /
 *
 * @return  The current stream volume.
 */
int8_t player_current_vol_get(uint8_t id);

/**
 * @brief  player_current_vol_set
 *      set the volume of the stream.
 *
 * @param[in] id The player stream id of the player volume. @see stream_id_t
 *           STREAM_MUSIC / STREAM_VOICE / STREAM_TONE /
 * @param[in]    db     The decibels of the player volume.
 *
 * @return 0 for success, else for the error code.
 */
uint8_t player_current_vol_set(uint8_t id, int8_t db);

/**
 * @brief  player_vol_force_gain_set
 *
 * @param[in] force true is set gain inoperative, false is operative.
 */
void player_vol_force_gain_set(bool force);

#endif //NEW_ARCH
#endif /* _SRC_LIB_PLAYER_H_ */
