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

#ifndef _SRC_LIB_PLAYER_VOICE_CORES_INC_NPLAYER_VOICE_ITL_H_
#define _SRC_LIB_PLAYER_VOICE_CORES_INC_NPLAYER_VOICE_ITL_H_

#include "types.h"
#include "modules.h"
#include "nplayer_voice_core.h"
#include "nplayer_voice_api.h"

/**
 * @brief  player_voice_create (opt rpc)
 *       malloc some resources to play voice.
 *
 * @param[in]    p_param      The pointer of the voice parameter.
 * @param[in]    is_opt_mic   True is operation,false is not.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_voice_call_create(player_voice_call_param_t *p_param, uint8_t is_opt_mic);

/**
 * @brief  player_voice_destroy (opt rpc)
 *       free some resources to play voice.
 *
 * @param[in]    reserve    The reserve param of the voice parameter.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_voice_call_destroy(uint32_t reserve);

#endif /* _SRC_LIB_PLAYER_VOICE_CORES_INC_NPLAYER_VOICE_ITL_H_ */
