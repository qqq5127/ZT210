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

#ifndef _AUDIO_SWEEP_SINE_TONE_H_
#define _AUDIO_SWEEP_SINE_TONE_H_

/**
 * @brief This function is to stop spk play sine tone.
 */
void audio_sweep_sine_tone_stop(void);

/**
 * @brief This function is to start spk play sine tone.
 * @param[in] freq play sine tone freq
 * @param[in] range play sine tone range
 */
void audio_sweep_sine_tone_start(uint16_t freq, uint16_t range);

/**
 * @brief This function is to start mic spk loop_back.
 * @param[in] bitmap record mic audio path id bitmap.
 * @param[in] reserved  for future use.
 * @return RET_OK return success else fail.
 */
uint8_t audio_sweep_loopback_start(uint8_t bitmap, uint8_t reserved);

/**
 * @brief audio_sweep_loopback_stop.
 * @return RET_OK return success else fail.
 */
uint8_t audio_sweep_loopback_stop(void);

#endif /* _AUDIO_SWEEP_SINE_TONE_H_ */
