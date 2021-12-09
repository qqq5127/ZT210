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

#ifndef _SRC_LIB_PLAYER_TONE_CORES_TONE_TX_DSP_H_
#define _SRC_LIB_PLAYER_TONE_CORES_TONE_TX_DSP_H__

#include "types.h"

/**
 * @brief start to send tone data to dsp.
 *
 * @param[in] addr to store tone data in flash.
 * @param[in] len of tone.
 */
void tone_data_start_send_dsp(uint32_t addr, uint32_t len);

/**
 * @brief stop to send tone data to dsp.
 */
void tone_data_stop_send_dsp(void);

#endif /* _SRC_LIB_PLAYER_TONE_CORES_TONE_TX_DSP_H__ */
