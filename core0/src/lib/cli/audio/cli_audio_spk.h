/****************************************************************************

Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
#ifndef __CLI_AUDIO_SPK_H_
#define __CLI_AUDIO_SPK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "iot_sdm_dac.h"

void cli_audio_spk_dig_gain_adjust_handler(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_spk_ana_gain_adjust_handler(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_spk_dcdc_1p2_modefy_handler(uint8_t *buffer, uint32_t bufferlen);

#ifdef __cplusplus
}
#endif

#endif
