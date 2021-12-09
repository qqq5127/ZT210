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

#ifndef _DRIVER_NON_OS_I2S_H
#define _DRIVER_NON_OS_I2S_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup I2S
 * @{
 * This section introduces the I2S module's enum, structure, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup hal_i2s_enum Enum
 * @{
 */

/** @brief I2S module. */
typedef enum {
    IOT_RX_I2S_MODULE,
    IOT_TX_I2S_MODULE,
} IOT_I2S_MODULE;

/** @brief I2S mclk. */
typedef enum {
    IOT_I2S_MCLK_16MHZ,
    IOT_I2S_MCLK_24MHZ,
    IOT_I2S_MCLK_32MHZ,
    IOT_I2S_MCLK_48MHZ,
}IOT_I2S_MCLK;

/** @brief I2S line id. */
typedef enum {
    IOT_I2S_LINE_0,
    IOT_I2S_LINE_1,
    IOT_I2S_LINE_2,
    IOT_I2S_LINE_3,
    IOT_I2S_LINE_MAX,
} IOT_I2S_LINE_ID;

/** @brief I2S work mode. */
typedef enum {
    IOT_I2S_PHILIPS_MODE,
    IOT_I2S_LEFT_JUSTIFIED_MODE,
    IOT_I2S_RIGHT_JUSTIFIED_MODE,
    IOT_I2S_WORK_MODE_MAX,
} IOT_I2S_WORK_MODE;

/** @brief I2S tdm work mode. */
typedef enum {
    IOT_I2S_STANDARD_MODE,
    IOT_I2S_TDM_MODE,
    IOT_I2S_ONE_SLOT_MODE,
    IOT_I2S_TDM_WORK_MODE_MAX,
} IOT_I2S_TDM_WORK_MODE;
/**
 * @}
 */

/**
 * @defgroup hal_i2s_struct Struct
 * @{
 */
typedef struct i2s_gpio_config {
    uint8_t ws;
    uint8_t bck;
    uint8_t data[IOT_I2S_LINE_MAX];
} i2s_gpio_cfg_t;
/**
 * @}
 */

/**
 * @brief This function is to init I2S driver.
 *
 */
void iot_i2s_init(void);

/**
 * @brief This function is to enable I2S apb mclk out.
 *
 */
void iot_i2s_mclk_out_enable(void);

/**
 * @brief This function is to disable I2S apb mclk out.
 *
 */
void iot_i2s_mclk_out_disable(void);

/**
 * @brief This function is to configure I2S work in dual-channel mode or single-channel mode.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @param single_channel means if I2S work in single-channel mode (default false).
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_set_single_channel(IOT_I2S_MODULE module,
                                   bool_t single_channel);

/**
 * @brief This function is to configure I2S work in master mode or slave mode.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @param slave_mode means if I2S work in slave mode (default false).
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_set_slave_mode(IOT_I2S_MODULE module, bool_t slave_mode);

/**
 * @brief This function is to configure I2S work mode.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @param mode is work mode defined by IOT_I2S_WORK_MODE (default philips mode).
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_set_work_mode(IOT_I2S_MODULE module, IOT_I2S_WORK_MODE mode);

/**
 * @brief This function is to configure I2S sample bit number and clock bit number.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @param sample_bits I2S sample bit number (default 16).
 * @param clk_bit_num I2S clock bit number (default 16),should not less than sample_bits.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_set_bit_mode(IOT_I2S_MODULE module, uint8_t sample_bits,
                             uint8_t clk_bit_num);

/**
 * @brief This function is to configure I2S sample frequence.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @param freq is sample frequence (default 44.1kHz)
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_set_sample_freq(IOT_I2S_MODULE module, uint32_t freq);

/**
 * @brief This function is to configure if I2S work in PCM mode.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @param pcm_mode means if I2S work in pcm mode (default false).
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_set_pcm_mode(IOT_I2S_MODULE module, bool_t pcm_mode);

/**
 * @brief This function is to configure I2S TDM work mode.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @param tdm_mode is tdm mode defined by IOT_I2S_TDM_WORK_MODE(default stardard mode).
 * @param chn_num is tdm channel number (default 2).
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_set_tdm_mode(IOT_I2S_MODULE module,
                             IOT_I2S_TDM_WORK_MODE tdm_mode, uint8_t chn_num);

/**
 * @brief This function is to configure if I2S send right channel data first.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @param right_first means if send right channel data first (default false).
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_set_right_first(IOT_I2S_MODULE module, bool_t right_first);

/**
 * @brief This function is to configure if I2S send msb right.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @param msb_right means if send msb right (default false).
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_set_msb_right(IOT_I2S_MODULE module, bool_t msb_right);

/**
 * @brief This function I2S rx mode to configure if I2S set 16 bit.
 *
 * @param set_16_bit means if set 16 bit (default false).
 */
void iot_i2s_set_16_bit_rx(bool_t set_16_bit);

/**
 * @brief This function is to write the configure into hardware.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_flush_config(IOT_I2S_MODULE module);

/**
 * @brief This function is to enable I2S data line.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @param line I2S data line number.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_enable_line(IOT_I2S_MODULE module, IOT_I2S_LINE_ID line);

/**
 * @brief This function is to disable I2S data line.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @param line I2S data line number.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_disable_line(IOT_I2S_MODULE module, IOT_I2S_LINE_ID line);

/**
 * @brief This function is to configure I2S gpio.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @param cfg I2S gpio define in i2s_gpio_cfg_t.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_set_gpio(IOT_I2S_MODULE module, const i2s_gpio_cfg_t *cfg);

/**
 * @brief This function is to enable I2S module.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_enable(IOT_I2S_MODULE module);

/**
 * @brief This function is to disable I2S module.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_disable(IOT_I2S_MODULE module);

/**
 * @brief This function is to reset I2S module.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_reset(IOT_I2S_MODULE module);

/**
 * @brief This function is to start I2S module.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_start(IOT_I2S_MODULE module);

/**
 * @brief This function is to stop I2S module.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_stop(IOT_I2S_MODULE module);

/**
 * @brief This function is to enable MCLK output of I2S.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_mclk_enable(IOT_I2S_MODULE module);

/**
 * @brief This function is to disable MCLK output of I2S.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_mclk_disable(IOT_I2S_MODULE module);

/**
 * @brief This function is to config GPIO for MCLK.
 *
 * @param module I2S module,RX I2S or TX I2S.
 * @param mclk I2S MCLK.
 * @param gpio I2S gpio.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2s_mclk_set_gpio(IOT_I2S_MODULE module, IOT_I2S_MCLK mclk, uint8_t gpio);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */
#endif /* _DRIVER_NON_OS_I2S_H */
