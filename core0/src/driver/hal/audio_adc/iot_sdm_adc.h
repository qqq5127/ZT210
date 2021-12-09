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

#ifndef _DRIVER_HAL_SDM_ADC_H
#define _DRIVER_HAL_SDM_ADC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IOT_RX_DFE_CHN_0,
    IOT_RX_DFE_CHN_1,
    IOT_RX_DFE_CHN_2,
    IOT_RX_DFE_CHN_3,
    IOT_RX_DFE_CHN_4,
    IOT_RX_DFE_CHN_5,
    IOT_RX_DFE_CHN_MAX,
    IOT_RX_DFE_CHN_NONE,
} IOT_RX_DFE_CHN_ID;

typedef enum {
    IOT_RX_DFE_ADC_0,
    IOT_RX_DFE_ADC_1,
    IOT_RX_DFE_ADC_2,
    IOT_RX_DFE_ADC_MAX,
} IOT_RX_DFE_ADC_ID;

typedef enum {
    IOT_RX_DFE_ADC,
    IOT_RX_DFE_PDM,
    IOT_RX_DFE_MAX,
} IOT_RX_DFE_MODE;

typedef enum {
    IOT_RX_DFE_FS_8K,
    IOT_RX_DFE_FS_16K,
    IOT_RX_DFE_FS_32K,
    IOT_RX_DFE_FS_48K,
    IOT_RX_DFE_FS_62K5,
    IOT_RX_DFE_FS_96K,
    IOT_RX_DFE_FS_192K,
    IOT_RX_DFE_FS_MAX,
} IOT_RX_DFE_FREQ_SAMPLING;

typedef enum {
    IOT_RX_DFE_ADC_IN,
    IOT_RX_DFE_ANC_SINC_OUT,
    IOT_RX_DFE_SINC_OUT,
    IOT_RX_DFE_HBF_OUT,
    IOT_RX_DFE_HPF_OUT,
    IOT_RX_DFE_PWR_SCALE_OUT,
    IOT_RX_DFE_ADC_OUT,
    IOT_RX_DFE_DUMP_DATA_MAX,
} IOT_RX_DFE_DUMP_DATA_MODULE;

typedef struct iot_rx_dfe_config {
    IOT_RX_DFE_MODE mode;
    IOT_RX_DFE_FREQ_SAMPLING fs;
} iot_rx_dfe_config_t;

/**
 * @brief This function is to enable sdm adc mclk.
 *
 */
void iot_sdm_adc_mclk_enable(void);

/**
 * @brief This function is to disable sdm adc mclk.
 *
 */
void iot_sdm_adc_mclk_disable(void);

/**
 * @brief This function is to enable adc mclk.
 *
 */
void iot_adc_mclk_enable(void);

/**
 * @brief This function is to disable adc mclk.
 *
 */
void iot_adc_mclk_disable(void);

/**
 * @brief This function is to enable rx dfe.
 *
 * @param chn is rx dfe channel id.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rx_dfe_enable(IOT_RX_DFE_CHN_ID chn);

/**
 * @brief This function is to init rx dfe
 *
 */
void iot_rx_dfe_init(void);

/**
 * @brief This function is to deinit rx dfe
 *
 */
void iot_rx_dfe_deinit(void);

/**
 * @brief This function is to claim rx dfe channel
 *
 * @return IOT_RX_DFE_CHN_ID
 */
uint8_t iot_rx_dfe_claim_channel(IOT_RX_DFE_CHN_ID chn);

/**
 * @brief This function is to release rx dfe channel
 *
 * @param chn is rx dfe channel id.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rx_dfe_release_channel(IOT_RX_DFE_CHN_ID chn);

/**
 * @brief enable multiple channels simultaneously for sync
 *
 * @param chn_matrix if enable channel, set the bit channel 1,else bit channel 0.
 * @param use_anc if use anc enable anc.
 * @return uint8_t uint8_t RET_OK for success else error.
 */
uint8_t iot_rx_dfe_sync_multipath(uint8_t chn_matrix, bool_t use_anc);

/**
 * @brief This function is to disable rx dfe.
 *
 * @param chn is rx dfe channel id.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rx_dfe_disable(IOT_RX_DFE_CHN_ID chn);

/**
 * @brief This function is to start rx dfe.
 *
 * @param chn is rx dfe channel id.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rx_dfe_start(IOT_RX_DFE_CHN_ID chn);

/**
 * @brief This function is to stop rx dfe.
 *
 * @param chn is rx dfe channel id.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rx_dfe_stop(IOT_RX_DFE_CHN_ID chn);

/**
 * @brief This function is to reset rx dfe.
 *
 * @param chn is rx dfe channel id.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rx_dfe_reset(IOT_RX_DFE_CHN_ID chn);

/**
 * @brief This function is rx dfe module link adc module.
 *
 * @param chn is rx dfe channel id.
 * @param adc_id is the adc id.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rx_dfe_link_adc(IOT_RX_DFE_CHN_ID chn, uint8_t adc_id);

/**
 * @brief This function is rx dfe module link pdm module.
 *
 * @param chn is rx dfe channel id.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rx_dfe_link_pdm(IOT_RX_DFE_CHN_ID chn);

/**
 * @brief This function is to call the rx dfe dump module.
 *
 * @param chn is rx dfe channel id.
 * @param sel is rx dfe dump data module.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rx_dfe_dump_module(IOT_RX_DFE_CHN_ID chn, IOT_RX_DFE_DUMP_DATA_MODULE sel);

/**
 * @brief enable hpf
 *
 * @param chn is rx dfe channel id.
 * @param enable
 */
void iot_rx_dfe_hpf_enable(IOT_RX_DFE_CHN_ID chn, bool_t enable);
/**
 * @brief This function is rx dfe to receive poll.
 *
 * @param chn is rx dfe channel id.
 * @return uint32_t is the data received from poll.
 */
uint32_t iot_rx_dfe_receive_poll(IOT_RX_DFE_CHN_ID chn);

/**
 * @brief This function is to set frequence for rx dfe.
 *
 * @param chn is rx dfe channel id.
 * @param cfg is the address of configuration.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rx_dfe_set_freq(IOT_RX_DFE_CHN_ID chn, const iot_rx_dfe_config_t *cfg);

/**
 * @brief This function is to config rx dfe.
 *
 * @param chn is rx dfe channel id.
 * @param rx_dfe_cfg is the address of configuration.
 */
void iot_rx_dfe_config(IOT_RX_DFE_CHN_ID chn, const iot_rx_dfe_config_t *rx_dfe_cfg);

/**
 * @brief This function is to call the rx dfe dump channel.
 *
 * @param chn is rx dfe channel id.
 */
void iot_rx_dfe_dump_channel(IOT_RX_DFE_CHN_ID chn);

/**
 * @brief This function is to let rx dfe set adc format.
 *
 * @param id is rx dfe sdm adc chn id.
 * @param format is the format value.bit 0: whether invert bits;bit 1: whether swap bits;bit 2: whether remove ½ LSB DC.
 */
void iot_rx_dfe_set_adc_format(IOT_RX_DFE_ADC_ID id, uint8_t format);

/**
 * @brief the gain pre step is 0.1875dB
 *
 * @param chn is rx dfe sdm adc chn id.
 * @param gain target gain
 */
void iot_rx_dfe_gain_set(IOT_RX_DFE_CHN_ID chn, int16_t gain);

#ifdef __cplusplus
}
#endif

#endif
