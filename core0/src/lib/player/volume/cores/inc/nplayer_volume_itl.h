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

#ifndef _SRC_LIB_PLAYER_VOLUME_CORES_INC_NPLAYER_VOLUME_ITL_H_
#define _SRC_LIB_PLAYER_VOLUME_CORES_INC_NPLAYER_VOLUME_ITL_H_

#ifdef NEW_ARCH
#include "types.h"
#include "modules.h"
#include "nplayer_volume_api.h"

/**
 * @brief player_volume_init
 *      player volume env init
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_volume_init(void);

/**
 * @brief  player_volume_resume
 *      resume the volume of the current stream.
 *
 * @param[in] id The player stream id of the player volume. @see stream_id_t
 *           STREAM_MUSIC / STREAM_VOICE / STREAM_TONE /
 */
void player_volume_resume(uint8_t id);
#endif

#endif /* _SRC_LIB_PLAYER_VOLUME_CORES_INC_NPLAYER_VOLUME_ITL_H_ */
