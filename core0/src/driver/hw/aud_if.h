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
#ifndef _DRIVER_HW_AUD_IF_H
#define _DRIVER_HW_AUD_IF_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* audio interface defines */
typedef enum {
    AUDIO_RX_FIFO_0,
    AUDIO_RX_FIFO_1,
    AUDIO_RX_FIFO_2,
    AUDIO_RX_FIFO_3,
    AUDIO_RX_FIFO_4,
    AUDIO_RX_FIFO_5,
    AUDIO_RX_FIFO_6,
    AUDIO_RX_FIFO_7,
    AUDIO_RX_FIFO_MAX,
} AUDIO_RX_FIFO_ID;

typedef enum {
    AUDIO_RX_ASRC_0,
    AUDIO_RX_ASRC_1,
    AUDIO_RX_ASRC_2,
    AUDIO_RX_ASRC_3,
    AUDIO_RX_ASRC_MAX,
} AUDIO_RX_ASRC_ID;

typedef enum {
    AUDIO_RX_DFE_0,
    AUDIO_RX_DFE_1,
    AUDIO_RX_DFE_2,
    AUDIO_RX_DFE_3,
    AUDIO_RX_DFE_4,
    AUDIO_RX_DFE_5,
    AUDIO_RX_DFE_MAX,
} AUDIO_RX_DFE_ID;

typedef enum {
    AUDIO_TX_FIFO_0,
    AUDIO_TX_FIFO_1,
    AUDIO_TX_FIFO_2,
    AUDIO_TX_FIFO_3,
    AUDIO_TX_FIFO_4,
    AUDIO_TX_FIFO_5,
    AUDIO_TX_FIFO_6,
    AUDIO_TX_FIFO_7,
    AUDIO_TX_FIFO_MAX,
} AUDIO_TX_FIFO_ID;

typedef enum {
    AUDIO_TX_ASRC_0,
    AUDIO_TX_ASRC_1,
    AUDIO_TX_ASRC_MAX,
} AUDIO_TX_ASRC_ID;

typedef enum {
    AUDIO_TX_DFE_0,
    AUDIO_TX_DFE_1,
    AUDIO_TX_DFE_DOUBLE,
    AUDIO_TX_DFE_MAX,
} AUDIO_TX_DFE_ID;

typedef enum {
    AUDIO_ANC_INPUT_MIC_0,
    AUDIO_ANC_INPUT_MIC_1,
    AUDIO_ANC_INPUT_MIC_2,
    AUDIO_ANC_INPUT_MIC_3,
    AUDIO_ANC_INPUT_MIC_4,
    AUDIO_ANC_INPUT_MIC_5,
    AUDIO_ANC_INPUT_ASRC_0,
    AUDIO_ANC_INPUT_ASRC_1,
    AUDIO_ANC_INPUT_MAX,
} AUDIO_ANC_INPUT_ID;

/* source selection defines */

typedef enum {
    AUDIO_ANC_RX_SRC_NONE,
    AUDIO_ANC_RX_SRC_DFE,
    AUDIO_ANC_RX_SRC_I2S,
    AUDIO_ANC_RX_SRC_MAX,
    AUDIO_ANC_RX_SRC_ASRC = AUDIO_ANC_RX_SRC_DFE,
} AUDIO_ANC_RX_SRC_ID;

typedef enum {
    AUDIO_RX_ASRC_SRC_DFE_0,
    AUDIO_RX_ASRC_SRC_DFE_1,
    AUDIO_RX_ASRC_SRC_DFE_2,
    AUDIO_RX_ASRC_SRC_DFE_3,
    AUDIO_RX_ASRC_SRC_DFE_4,
    AUDIO_RX_ASRC_SRC_DFE_5,
    AUDIO_RX_ASRC_SRC_I2S_0,
    AUDIO_RX_ASRC_SRC_I2S_1,
    AUDIO_RX_ASRC_SRC_I2S_2,
    AUDIO_RX_ASRC_SRC_I2S_3,
    AUDIO_RX_ASRC_SRC_I2S_4,
    AUDIO_RX_ASRC_SRC_I2S_5,
    AUDIO_RX_ASRC_SRC_I2S_6,
    AUDIO_RX_ASRC_SRC_I2S_7,
    AUDIO_RX_ASRC_SRC_MAX,
    AUDIO_RX_ASRC_SRC_NONE = 0x0f,
} AUDIO_RX_ASRC_SRC_ID;

typedef enum {
    AUDIO_SDM_ADC_0,
    AUDIO_SDM_ADC_1,
    AUDIO_SDM_ADC_2,
    AUDIO_SDM_ADC_MAX,
} AUDIO_SDM_ADC_ID;

typedef enum {
    AUDIO_RX_DEF_ADC_0,
    AUDIO_RX_DEF_ADC_1,
    AUDIO_RX_DEF_ADC_2,
    AUDIO_RX_DEF_PDM,
    AUDIO_RX_DEF_SRC_MAX,
} AUDIO_RX_DEF_SRC_ID;

typedef enum {
    AUDIO_TX_DEF_SRC_DFE_0,
    AUDIO_TX_DEF_SRC_DFE_1,
    AUDIO_TX_DEF_SRC_DFE_2,
    AUDIO_TX_DEF_SRC_DFE_3,
    AUDIO_TX_DEF_SRC_DFE_4,
    AUDIO_TX_DEF_SRC_DFE_5,
    AUDIO_TX_DEF_SRC_DFE_MAX,
    AUDIO_TX_DEF_SRC_ASRC = AUDIO_TX_DEF_SRC_DFE_MAX,
    AUDIO_TX_DEF_SRC_ANC,
    AUDIO_TX_DEF_SRC_MAX,
} AUDIO_TX_DEF_SRC_ID;

typedef enum {
    AUDIO_MIX_MODE_LR,
    AUDIO_MIX_MODE_RL,
    AUDIO_MIX_MODEAVG,
    AUDIO_MIX_MODE_RR,
    AUDIO_MIX_MODE_MAX
} AUDIO_MIX_MODE;

typedef enum {
    AUDIO_RX_FIFO_SRC_I2S_RX_0,
    AUDIO_RX_FIFO_SRC_I2S_RX_1,
    AUDIO_RX_FIFO_SRC_I2S_RX_2,
    AUDIO_RX_FIFO_SRC_I2S_RX_3,
    AUDIO_RX_FIFO_SRC_I2S_RX_4,
    AUDIO_RX_FIFO_SRC_I2S_RX_5,
    AUDIO_RX_FIFO_SRC_I2S_RX_6,
    AUDIO_RX_FIFO_SRC_I2S_RX_7,
    AUDIO_RX_FIFO_SRC_ASRC_0,
    AUDIO_RX_FIFO_SRC_ASRC_1,
    AUDIO_RX_FIFO_SRC_ASRC_2,
    AUDIO_RX_FIFO_SRC_ASRC_3,
    AUDIO_RX_FIFO_SRC_VAD,
    AUDIO_RX_FIFO_SRC_ADC,
    AUDIO_RX_FIFO_SRC_MAX,
} AUDIO_RX_FIFO_SRC_ID;

typedef enum {
    AUDIO_TX_FIFO_SRC_I2S_TX_0,
    AUDIO_TX_FIFO_SRC_I2S_TX_1,
    AUDIO_TX_FIFO_SRC_I2S_TX_2,
    AUDIO_TX_FIFO_SRC_I2S_TX_3,
    AUDIO_TX_FIFO_SRC_I2S_TX_4,
    AUDIO_TX_FIFO_SRC_I2S_TX_5,
    AUDIO_TX_FIFO_SRC_I2S_TX_6,
    AUDIO_TX_FIFO_SRC_I2S_TX_7,
    AUDIO_TX_FIFO_SRC_DFE_TX_0,
    AUDIO_TX_FIFO_SRC_DFE_TX_1,
    AUDIO_TX_FIFO_SRC_MAX,
} AUDIO_TX_FIFO_SRC_ID;

typedef enum {
    AUDIO_TIMER_TARGET_0,
    AUDIO_TIMER_TARGET_1,
    AUDIO_TIMER_TARGET_2,
    AUDIO_TIMER_TARGET_3,
    AUDIO_TIMER_TARGET_MAX,
} AUDIO_TIMER_TARGET_ID;

typedef enum {
    AUDIO_ASRC_START_BY_TIMER,
    AUDIO_ASRC_START_BY_BT,
    AUDIO_ASRC_START_NO_HW_SRC,
} AUDIO_ASRC_START_HW_SRC;

typedef enum {
    AUDIO_ASRC_LATCH_BY_TIMER,
    AUDIO_ASRC_LATCH_BY_BT,
    AUDIO_ASRC_LATCH_NO_HW_SRC,
} AUDIO_ASRC_LATCH_HW_SRC;

typedef enum {
    AUDIO_LINK_ASRC_NONE = 0,
    AUDIO_LINK_ASRC_0 = 8,
    AUDIO_LINK_ASRC_1 = 9,
    AUDIO_LINK_ASRC_2 = 10,
    AUDIO_LINK_ASRC_3 = 11,
    AUDIO_LINK_ASRC_MAX,
}AUDIO_RXFIFO_LINK_ASRC_ID;

/* function defines */
void audio_fifo_init(void);
void audio_fifo_deinit(void);
void audio_set_out_sel(uint8_t sel);
void audio_set_anc_src(AUDIO_ANC_INPUT_ID id, AUDIO_ANC_RX_SRC_ID src);
void audio_set_rx_asrc_src(AUDIO_RX_ASRC_ID id, AUDIO_RX_ASRC_SRC_ID src);
void audio_asrc_bt_select(AUDIO_RX_ASRC_ID id);
void audio_enable_asrc_bt_tgt(bool_t enable);
void audio_set_rx_dfe_src(AUDIO_RX_DFE_ID id, AUDIO_RX_DEF_SRC_ID src);
void audio_set_tx_dfe_mix_mode(AUDIO_MIX_MODE mode);
void audio_set_tx_dfe_src(AUDIO_TX_DFE_ID id, AUDIO_TX_DEF_SRC_ID src);
void audio_set_sdm_adc_format(AUDIO_SDM_ADC_ID id, uint8_t format);
void audio_set_bt_latch_overwrite(uint8_t latch);
void audio_set_i2s_16_bit(bool_t set);
void audio_set_bt_start_asrc_overwrite(uint8_t start);

/**
 * @brief When multiple fifo need to link multiple asrc at the same time,
 * they need to keep the corresponding bit position 1 of the aif_rxfifo_ctrl
 * at the same time, but the module that does not need to link remains the
 * same, and the user needs to provide the corresponding bit information of
 * the fifo and the asrc
 *
 * @param rx_fifo_matrix each rx fifo id represents binary one bit
 */
void audio_set_rx_fifo_link_asrc_multipath(const AUDIO_RXFIFO_LINK_ASRC_ID *rx_fifo_matrix);
void audio_set_rx_fifo_link_dfe_multipath(uint8_t rx_fifo_matrix);
void audio_set_rx_fifo_dislink_multipath(uint8_t rx_fifo_matrix);
void audio_set_rx_fifo_src(AUDIO_RX_FIFO_ID id, AUDIO_RX_FIFO_SRC_ID src);
void audio_enable_rx_fifo(AUDIO_RX_FIFO_ID id);
void audio_disable_rx_fifo(AUDIO_RX_FIFO_ID id);
void audio_reset_rx_fifo(AUDIO_RX_FIFO_ID id);
void audio_enable_rx_fifo_half_word_mode(bool_t enable);
void audio_set_tx_fifo_src(AUDIO_TX_FIFO_ID id, AUDIO_TX_FIFO_SRC_ID src);
void audio_enable_tx_fifo(AUDIO_TX_FIFO_ID id);
void audio_disable_tx_fifo(AUDIO_TX_FIFO_ID id);
void audio_reset_tx_fifo(AUDIO_TX_FIFO_ID id);

void audio_timer_enable(bool_t enable);
void audio_timer_matrix_enable(uint8_t timer_target_matrix);
void audio_timer_enable_target(AUDIO_TIMER_TARGET_ID id, bool_t enable);
void audio_timer_set_target(AUDIO_TIMER_TARGET_ID id, uint32_t value);
uint32_t audio_timer_get_freerun_tick(void);
uint32_t audio_timer_get_latched_tick(void);
void audio_set_asrc_hw_start_src(uint8_t asrc_id, AUDIO_ASRC_START_HW_SRC src);
void audio_set_asrc_hw_latch_src(uint8_t asrc_id, AUDIO_ASRC_LATCH_HW_SRC src);

uint32_t audio_get_rx_fifo_dma_addr(AUDIO_RX_FIFO_ID id);
uint32_t audio_get_tx_fifo_dma_addr(AUDIO_TX_FIFO_ID id);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_AUD_IF_H */
