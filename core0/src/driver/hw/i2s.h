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
#ifndef _DRIVER_HW_IIS_H
#define _DRIVER_HW_IIS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2S_FREQUENCE_DIV_NUM_4 0x04

typedef enum {
    I2S_RX_LINE_0,
    I2S_RX_LINE_1,
    I2S_RX_LINE_2,
    I2S_RX_LINE_3,
    I2S_RX_LINE_MAX,
    I2S_RX_LINE_NONE = I2S_RX_LINE_MAX,
} I2S_RX_LINE_ID;

typedef enum {
    I2S_TX_LINE_0,
    I2S_TX_LINE_1,
    I2S_TX_LINE_2,
    I2S_TX_LINE_3,
    I2S_TX_LINE_MAX,
    I2S_TX_LINE_NONE = I2S_TX_LINE_MAX,
} I2S_TX_LINE_ID;

typedef enum {
    I2S_RX_DATA_CHANNEL_0,
    I2S_RX_DATA_CHANNEL_1,
    I2S_RX_DATA_CHANNEL_2,
    I2S_RX_DATA_CHANNEL_3,
    I2S_RX_DATA_CHANNEL_4,
    I2S_RX_DATA_CHANNEL_5,
    I2S_RX_DATA_CHANNEL_6,
    I2S_RX_DATA_CHANNEL_7,
    I2S_RX_DATA_CHANNEL_MAX,
    I2S_RX_DATA_CHANNEL_NONE = I2S_RX_DATA_CHANNEL_MAX,
} I2S_RX_DATA_CHANNEL_ID;

typedef enum {
    I2S_TX_DATA_CHANNEL_0,
    I2S_TX_DATA_CHANNEL_1,
    I2S_TX_DATA_CHANNEL_2,
    I2S_TX_DATA_CHANNEL_3,
    I2S_TX_DATA_CHANNEL_4,
    I2S_TX_DATA_CHANNEL_5,
    I2S_TX_DATA_CHANNEL_6,
    I2S_TX_DATA_CHANNEL_7,
    I2S_TX_DATA_CHANNEL_MAX,
    I2S_TX_DATA_CHANNEL_NONE = I2S_TX_DATA_CHANNEL_MAX,
} I2S_TX_DATA_CHANNEL_ID;

typedef enum {
    I2S_STANDARD_MODE,
    I2S_TDM_MODE,
    I2S_ONE_SLOT_MODE,
} tdm_mode_t;

typedef struct i2s_tdm_config {
    tdm_mode_t  tdm_mode;
    uint8_t tdm_chn_num;    /* the number of tdm channel, default 2 */
} i2s_tdm_cfg_t;

typedef enum {
    I2S_PHILIPS_MODE,
    I2S_LEFT_JUSTIFIED_MODE,
    I2S_RIGTH_JUSTIFIED_MODE,
    I2S_INVALID_WORK_MODE,
} I2S_MODE;

typedef enum {
    I2S_BITS_16BIT = 15,
    I2S_BITS_24BIT = 23,
    I2S_BITS_32BIT = 31,
} i2s_sample_bits_t;

typedef struct i2s_config {
    bool_t single_chan;    /* false : dual channel ;  ture : single channel */
    bool_t slave_mode;    /* false : master mode ;  ture : slave mode */
    I2S_MODE i2s_mode;
    bool_t pcm_mode;     /* false : normal ;  ture : pcm data format */
    i2s_tdm_cfg_t tdm_cfg;
    uint32_t sample_freq;
    i2s_sample_bits_t bits_mode;
    uint8_t bit_clk_num;
    bool_t right_channel_fist; /* false : left first ;  ture : right first */
    bool_t msb_right;
    //bool_t extern_start;
} i2s_cfg_t;


void i2s_get_default_config(i2s_cfg_t *cfg);
void i2s_config_rx(const i2s_cfg_t *rx_cfg);
void i2s_enable_rx_line(I2S_RX_LINE_ID ch);
void i2s_disable_rx_line(I2S_RX_LINE_ID ch);
void i2s_get_rx_data_channel(I2S_RX_LINE_ID ch,
        I2S_RX_DATA_CHANNEL_ID *left, I2S_RX_DATA_CHANNEL_ID *right);
void i2s_config_rx_reset(void);
void i2s_config_rx_start(void);
void i2s_config_rx_stop(void);
void i2s_config_tx(const i2s_cfg_t *tx_cfg);
void i2s_enable_tx_line(I2S_TX_LINE_ID ch);
void i2s_disable_tx_line(I2S_TX_LINE_ID ch);
void i2s_get_tx_data_channel(I2S_TX_LINE_ID ch,
        I2S_TX_DATA_CHANNEL_ID *left, I2S_TX_DATA_CHANNEL_ID *right);
void i2s_config_tx_reset(void);
void i2s_config_tx_start(void);
void i2s_config_tx_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_IIS_H */
