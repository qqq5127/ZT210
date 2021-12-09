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

#ifndef _AUDIO_ANC_SWEEP_CHIRP_H_
#define _AUDIO_ANC_SWEEP_CHIRP_H_

// change to int
#define SWEEP_CHIRP_SAMPLING_RATE 160
#define SWEEP_CHIRP_SAMPLING_TIME 48
#define SWEEP_CHIRP_SAMPLING_TOTAL_NUMBER (SWEEP_CHIRP_SAMPLING_TIME * SWEEP_CHIRP_SAMPLING_RATE) //word number
#define SWEEP_CHIRP_RECORD_LEN (RING_ORIGIN_LENGTH / 2 - SWEEP_CHIRP_SAMPLING_TOTAL_NUMBER)

void audio_anc_sweep_chirp_start(void);
void audio_anc_sweep_chirp_stop(void);
void audio_anc_sweep_chirp_init(uint8_t mic_bitmap);
void audio_anc_tdd_noise_dump_start(uint8_t mic_idx, uint8_t bit16_mode);
void audio_sweep_mic_deinit(uint8_t fifo_id, uint8_t mic_id, uint8_t chan_id, uint8_t asrc_id);
void audio_sweep_spk_deinit(uint8_t asrc_id, uint8_t dac_chn);
void audio_sweep_spk_config(uint8_t src);

#endif
