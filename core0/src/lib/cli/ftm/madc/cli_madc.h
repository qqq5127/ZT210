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

#ifndef LIB_CLI_MADC_H
#define LIB_CLI_MADC_H

void cli_madc_start_handler(uint8_t *buffer, uint32_t bufferlen);
void cli_madc_stop_handler(uint8_t *buffer, uint32_t bufferlen);
void cli_madc_init_handler(uint8_t *buffer, uint32_t bufferlen);
void cli_madc_set_channel_handler(uint8_t *buffer, uint32_t bufferlen);
void cli_madc_poll_data_handler(uint8_t *buffer, uint32_t bufferlen);
void cli_madc_dump_handler(uint8_t *buffer, uint32_t bufferlen);
void audio_run(void);
void cli_madc_set_audio_gain_handler(uint8_t *buffer, uint32_t bufferlen);

#endif /* LIB_CLI_MADC_H */
