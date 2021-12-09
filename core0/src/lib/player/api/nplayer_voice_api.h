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

#ifndef _SRC_LIB_PLAYER_API_NPLAYER_VOICE_API_H_
#define _SRC_LIB_PLAYER_API_NPLAYER_VOICE_API_H_

#include "types.h"
#include "nplayer_api.h"

typedef struct _player_voice_call_param_t {
    //codec info
    player_codec_t codec;
    //volume unit db
    int8_t vol;
} player_voice_call_param_t;
#ifdef NEW_ARCH

/**
 * @brief player_voice_call_init.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_voice_call_init(void);

/**
 * @brief  player_voice_call_start
 *       start play voice.
 *
 * @param[in]    p_param      The pointer of the voice parameter.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_voice_call_start(player_voice_call_param_t *p_param);

/**
 * @brief This function is to config voicel.
 *
 * @param link_type is type of voice.
 * @param air_mode
 * @param rx_len
 * @param tx_len
 */
void player_voice_call_sco_cfg(uint8_t link_type, uint8_t air_mode, uint16_t rx_len, uint16_t tx_len);

/**
 * @brief  player_voice_call_stop
 *       stop play voice.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_voice_call_stop(void);

/**
 * @brief This function is to receive voice data.
 *
 * @param p_dat is the pointer of the voice data.
 * @param length is the length of the voice data.
 * @return uint8_t RET_OK for success else error.
 */

uint16_t player_voice_call_in(uint8_t *p_dat, uint16_t length);
#endif // NEW_ARCH
#endif /* _SRC_LIB_PLAYER_API_NPLAYER_VOICE_API_H_ */
