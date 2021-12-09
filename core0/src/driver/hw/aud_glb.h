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
#ifndef _DRIVER_HW_AUD_GLB_H
#define _DRIVER_HW_AUD_GLB_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AUDIO_DUMP_ADC_0,
    AUDIO_DUMP_ADC_1,
    AUDIO_DUMP_ADC_2,
    AUDIO_DUMP_ADC_3,
    AUDIO_DUMP_ADC_4,
    AUDIO_DUMP_ADC_5,
    AUDIO_DUMP_NONE,
    AUDIO_DUMP_DAC_0,
    AUDIO_DUMP_DAC_1,
    AUDIO_DUMP_MAX,
}AUDIO_DUMP_SRC;

typedef enum {
    AUDIO_MODULE_I2S_RX,
    AUDIO_MODULE_I2S_TX,
    AUDIO_MODULE_ADC_0,
    AUDIO_MODULE_ADC_1,
    AUDIO_MODULE_ADC_2,
    AUDIO_MODULE_ADC_3,
    AUDIO_MODULE_ADC_4,
    AUDIO_MODULE_ADC_5,
    AUDIO_MODULE_DAC_0,
    AUDIO_MODULE_DAC_1,
    AUDIO_MODULE_PDM_RX_0,
    AUDIO_MODULE_PDM_RX_1,
    AUDIO_MODULE_PDM_RX_2,
    AUDIO_MODULE_ASRC_0,
    AUDIO_MODULE_ASRC_1,
    AUDIO_MODULE_ASRC_2,
    AUDIO_MODULE_ASRC_3,
    AUDIO_MODULE_VAD,
    AUDIO_MODULE_METER_ADC,
    AUDIO_MODULE_CLK_I2S_RX,
    AUDIO_MODULE_CLK_I2S_TX,
    AUDIO_MODULE_MCLK_DAC,
    AUDIO_MODULE_MCLK_ADC,
    AUDIO_MODULE_ANC,
    AUDIO_MODULE_DMA_RX,
    AUDIO_MODULE_DMA_TX,
    AUDIO_MODULE_MCLK_RX,
    AUDIO_MODULE_MCLK_TX,
    AUDIO_MODULE_MAX,
} AUDIO_MODULE_ID;

typedef enum {
    AUDIO_MISC_CLK_EB_PCLK_0,
    AUDIO_MISC_CLK_EB_PCLK_1,
    AUDIO_MISC_CLK_EB_PCLK_2,
    AUDIO_MISC_CLK_EB_PCLK_3,
    AUDIO_MISC_CLK_EB_PCLK_4,
    AUDIO_MISC_CLK_EB_PCLK_5,
    AUDIO_MISC_CLK_EB_PCLK_6,
    AUDIO_MISC_CLK_EB_PCLK_7,
    AUDIO_MISC_CLK_EB_PCLK_8,
    AUDIO_MISC_CLK_EB_PCLK_9,
    AUDIO_MISC_CLK_EB_PCLK_10,
    AUDIO_MISC_CLK_EB_PCLK_11,
    AUDIO_MISC_CLK_EB_PCLK_12,
    AUDIO_MISC_CLK_EB_HCLK,
    AUDIO_MISC_CLK_EB_MAX
} AUDIO_MISC_CLK_EB;

/* function defines */
uint32_t audio_get_global_clk_status(void);
void audio_cfg_multipath(AUDIO_MODULE_ID id);
void audio_release_multipath(AUDIO_MODULE_ID id);
void audio_multipath_enable(void);
void audio_enable_module(AUDIO_MODULE_ID module);
void audio_disable_module(AUDIO_MODULE_ID module);
void audio_reset_module(AUDIO_MODULE_ID module);
void audio_misc_clk_enable_module(AUDIO_MISC_CLK_EB module);
void audio_misc_clk_disable_module(AUDIO_MISC_CLK_EB module);
void audio_misc_clk_reset_module(AUDIO_MISC_CLK_EB module);
void audio_dump_dac_select(AUDIO_DUMP_SRC sel);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_AUD_GLB_H */
