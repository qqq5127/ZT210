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

#ifndef LIB_CLI_AUDIO_ANC_H
#define LIB_CLI_AUDIO_ANC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief to do
 *
 * @param buffer
 * @param bufferlen
 */
void cli_audio_anc_set_coeff_param(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_anc_dump_init(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_anc_dump_start(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_anc_dump_stop(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_anc_play_sweep_start(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_anc_play_sweep_stop(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_anc_write_coeff_to_flash(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_anc_check_coeff_from_flash(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_anc_read_coeff_from_flash(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_anc_port_open(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_anc_port_close(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_anc_sweep_init(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_anc_secret_handshake(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_sweep_sine_tone_start(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_sweep_sine_tone_stop(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_anc_tdd_noise_dump_start(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_dump_mode_set(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_sweep_loopback_start(uint8_t *buffer, uint32_t bufferlen);
void cli_audio_sweep_loopback_stop(uint8_t *buffer, uint32_t bufferlen);
void cli_spp_audio_dump_param_set(uint8_t *buffer, uint32_t bufferlen);
#ifdef __cplusplus
}
#endif

#endif /* LIB_CLI_AUDIO_ANC_H */
