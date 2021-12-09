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
/* common includes */
#include "types.h"

/* hw includes */
#include "aud_glb.h"
#include "aud_if.h"
#include "i2s.h"
#include "apb.h"
#include "gpio.h"
#include "gpio_mtx.h"

#include "iot_audio.h"
#include "iot_i2s.h"
#include "aud_intf_pwr.h"
static i2s_cfg_t rx_i2s_cfg;
static i2s_cfg_t tx_i2s_cfg;

void iot_i2s_init(void)
{
    i2s_get_default_config(&rx_i2s_cfg);
    i2s_get_default_config(&tx_i2s_cfg);
}

void iot_i2s_mclk_out_enable(void)
{
    apb_mclk_out_enable();
}

void iot_i2s_mclk_out_disable(void)
{
    apb_mclk_out_disable();
}

uint8_t iot_i2s_set_single_channel(IOT_I2S_MODULE module, bool_t single_channel)
{
    if (module == IOT_RX_I2S_MODULE) {
        rx_i2s_cfg.single_chan = single_channel;
    } else if (module == IOT_TX_I2S_MODULE) {
        tx_i2s_cfg.single_chan = single_channel;
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_set_slave_mode(IOT_I2S_MODULE module, bool_t slave_mode)
{
    if (module == IOT_RX_I2S_MODULE) {
        rx_i2s_cfg.slave_mode = slave_mode;
    } else if (module == IOT_TX_I2S_MODULE) {
        tx_i2s_cfg.slave_mode = slave_mode;
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_set_work_mode(IOT_I2S_MODULE module, IOT_I2S_WORK_MODE mode)
{
    if (mode >= IOT_I2S_WORK_MODE_MAX) {
        return RET_INVAL;
    }
    if (module == IOT_RX_I2S_MODULE) {
        rx_i2s_cfg.i2s_mode = (I2S_MODE)mode;
    } else if (module == IOT_TX_I2S_MODULE) {
        tx_i2s_cfg.i2s_mode = (I2S_MODE)mode;
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_set_bit_mode(IOT_I2S_MODULE module, uint8_t sample_bits,
                             uint8_t clk_bit_num)
{
    i2s_cfg_t *cfg;

    if (sample_bits > clk_bit_num) {
        return RET_INVAL;
    }

    if (module == IOT_RX_I2S_MODULE) {
        cfg = &rx_i2s_cfg;
    } else if (module == IOT_TX_I2S_MODULE) {
        cfg = &tx_i2s_cfg;
    } else {
        return RET_INVAL;
    }

    if (sample_bits == 16) {
        cfg->bits_mode = I2S_BITS_16BIT;
    } else if (sample_bits == 24) {
        cfg->bits_mode = I2S_BITS_24BIT;
    } else if (sample_bits == 32) {
        cfg->bits_mode = I2S_BITS_32BIT;
    } else {
        return RET_INVAL;
    }

    cfg->bit_clk_num = clk_bit_num;

    return RET_OK;
}

uint8_t iot_i2s_set_sample_freq(IOT_I2S_MODULE module, uint32_t freq)
{
    if (module == IOT_RX_I2S_MODULE) {
        rx_i2s_cfg.sample_freq = freq;
    } else if (module == IOT_TX_I2S_MODULE) {
        tx_i2s_cfg.sample_freq = freq;
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_set_pcm_mode(IOT_I2S_MODULE module, bool_t pcm_mode)
{
    if (module == IOT_RX_I2S_MODULE) {
        rx_i2s_cfg.pcm_mode = pcm_mode;
    } else if (module == IOT_TX_I2S_MODULE) {
        tx_i2s_cfg.pcm_mode = pcm_mode;
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_set_tdm_mode(IOT_I2S_MODULE module,
                             IOT_I2S_TDM_WORK_MODE tdm_mode, uint8_t chn_num)
{
    if (tdm_mode >= IOT_I2S_TDM_WORK_MODE_MAX) {
        return RET_INVAL;
    }

    if (tdm_mode == IOT_I2S_STANDARD_MODE) {
        chn_num = 2;
    }

    if (module == IOT_RX_I2S_MODULE) {
        rx_i2s_cfg.tdm_cfg.tdm_mode = (tdm_mode_t)tdm_mode;
        rx_i2s_cfg.tdm_cfg.tdm_chn_num = chn_num;
    } else if (module == IOT_TX_I2S_MODULE) {
        tx_i2s_cfg.tdm_cfg.tdm_mode = (tdm_mode_t)tdm_mode;
        tx_i2s_cfg.tdm_cfg.tdm_chn_num = chn_num;
    } else {
        return RET_INVAL;
    }

    return RET_OK;
}

uint8_t iot_i2s_set_right_first(IOT_I2S_MODULE module, bool_t right_first)
{
    if (module == IOT_RX_I2S_MODULE) {
        rx_i2s_cfg.right_channel_fist = right_first;
    } else if (module == IOT_TX_I2S_MODULE) {
        tx_i2s_cfg.right_channel_fist = right_first;
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_set_msb_right(IOT_I2S_MODULE module, bool_t msb_right)
{
    if (module == IOT_RX_I2S_MODULE) {
        rx_i2s_cfg.msb_right = msb_right;
    } else if (module == IOT_TX_I2S_MODULE) {
        tx_i2s_cfg.msb_right = msb_right;
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_flush_config(IOT_I2S_MODULE module)
{
    if (module == IOT_RX_I2S_MODULE) {
        i2s_config_rx(&rx_i2s_cfg);
    } else if (module == IOT_TX_I2S_MODULE) {
        i2s_config_tx(&tx_i2s_cfg);
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

void iot_i2s_set_16_bit_rx(bool_t set_16_bit)
{
    audio_set_i2s_16_bit(set_16_bit);
}

uint8_t iot_i2s_enable_line(IOT_I2S_MODULE module, IOT_I2S_LINE_ID line)
{
    if (line >= IOT_I2S_LINE_MAX) {
        return RET_INVAL;
    }
    if (module == IOT_RX_I2S_MODULE) {
        i2s_enable_rx_line((I2S_RX_LINE_ID)line);
    } else if (module == IOT_TX_I2S_MODULE) {
        i2s_enable_tx_line((I2S_TX_LINE_ID)line);
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_disable_line(IOT_I2S_MODULE module, IOT_I2S_LINE_ID line)
{
    if (line >= IOT_I2S_LINE_MAX) {
        return RET_INVAL;
    }
    if (module == IOT_RX_I2S_MODULE) {
        i2s_disable_rx_line((I2S_RX_LINE_ID)line);
    } else if (module == IOT_TX_I2S_MODULE) {
        i2s_disable_tx_line((I2S_TX_LINE_ID)line);
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_set_gpio(IOT_I2S_MODULE module, const i2s_gpio_cfg_t *cfg)
{
    uint16_t pin[2 + IOT_I2S_LINE_MAX];
    uint16_t i;
    uint16_t pin_num;

    pin[0] = (uint16_t)cfg->bck;
    pin[1] = (uint16_t)cfg->ws;
    pin_num = 2;
    for (i = 0; i < IOT_I2S_LINE_MAX; i++) {
        if ((cfg->data[i] > 0) && (cfg->data[i] < 0xff)) {
            pin[pin_num] = (uint16_t)cfg->data[i];
            pin_num++;
        }
    }

    if (gpio_claim_group(pin, pin_num, true) != RET_OK) {
        return RET_INVAL;
    }

    /* config gpio matrix */
    if (module == IOT_RX_I2S_MODULE) {
        if (rx_i2s_cfg.slave_mode) {
            gpio_mtx_set_in_signal(cfg->bck, GPIO_MTX_IIS_I_BCK_IN,
                                   GPIO_MTX_MODE_MATRIX);
            gpio_mtx_set_in_signal(cfg->ws, GPIO_MTX_IIS_I_WS_IN,
                                   GPIO_MTX_MODE_MATRIX);
        } else {
            gpio_mtx_set_out_signal(cfg->bck, GPIO_MTX_IIS_I_BCK_OUT);
            gpio_mtx_set_out_signal(cfg->ws, GPIO_MTX_IIS_I_WS_OUT);
        }

        for (IOT_I2S_LINE_ID j = IOT_I2S_LINE_0; j < IOT_I2S_LINE_MAX; j++) {
            if ((cfg->data[j] > 0) && (cfg->data[j] < 0xff)) {
                gpio_mtx_set_in_signal(cfg->data[j],
                                       GPIO_MTX_IIS_I_DATA0_IN + (GPIO_MTX_SIGNAL_IN)j,
                                       GPIO_MTX_MODE_MATRIX);
            }
        }
    } else if (module == IOT_TX_I2S_MODULE) {
        if (tx_i2s_cfg.slave_mode) {
            gpio_mtx_set_in_signal(cfg->bck, GPIO_MTX_IIS_O_BCK_IN,
                                   GPIO_MTX_MODE_MATRIX);
            gpio_mtx_set_in_signal(cfg->ws, GPIO_MTX_IIS_O_WS_IN,
                                   GPIO_MTX_MODE_MATRIX);
        } else {
            gpio_mtx_set_out_signal(cfg->bck, GPIO_MTX_IIS_O_BCK_OUT);
            gpio_mtx_set_out_signal(cfg->ws, GPIO_MTX_IIS_O_WS_OUT);
        }

        for (IOT_I2S_LINE_ID j = IOT_I2S_LINE_0; j < IOT_I2S_LINE_MAX; j++) {
            if ((cfg->data[j] > 0) && (cfg->data[j] < 0xff)) {
                gpio_mtx_set_out_signal(cfg->data[j],
                                        GPIO_MTX_IIS_O_DATA0_OUT + (GPIO_MTX_SIGNAL_OUT)j);
            }
        }
    } else {
        return RET_INVAL;
    }

    return RET_OK;
}

uint8_t iot_i2s_enable(IOT_I2S_MODULE module)
{
    if (module == IOT_RX_I2S_MODULE) {
        //audio intf power on vote
        aud_intf_pwr_on(AUDIO_MODULE_I2S_RX);
        audio_enable_module(AUDIO_MODULE_I2S_RX);
        audio_enable_module(AUDIO_MODULE_CLK_I2S_RX);
    } else if (module == IOT_TX_I2S_MODULE) {
        //audio intf power on vote
        aud_intf_pwr_on(AUDIO_MODULE_I2S_TX);
        audio_enable_module(AUDIO_MODULE_I2S_TX);
        audio_enable_module(AUDIO_MODULE_CLK_I2S_TX);
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_disable(IOT_I2S_MODULE module)
{
    if (module == IOT_RX_I2S_MODULE) {
        audio_disable_module(AUDIO_MODULE_I2S_RX);
        audio_disable_module(AUDIO_MODULE_CLK_I2S_RX);
        //audio intf power off vote
        aud_intf_pwr_off(AUDIO_MODULE_I2S_RX);
    } else if (module == IOT_TX_I2S_MODULE) {
        audio_disable_module(AUDIO_MODULE_I2S_TX);
        audio_disable_module(AUDIO_MODULE_CLK_I2S_TX);
        //audio intf power off vote
        aud_intf_pwr_off(AUDIO_MODULE_I2S_TX);
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_reset(IOT_I2S_MODULE module)
{
    if (module == IOT_RX_I2S_MODULE) {
        audio_reset_module(AUDIO_MODULE_I2S_RX);
        audio_reset_module(AUDIO_MODULE_CLK_I2S_RX);
        i2s_get_default_config(&rx_i2s_cfg);
    } else if (module == IOT_TX_I2S_MODULE) {
        audio_reset_module(AUDIO_MODULE_I2S_TX);
        audio_reset_module(AUDIO_MODULE_CLK_I2S_TX);
        i2s_get_default_config(&tx_i2s_cfg);
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_start(IOT_I2S_MODULE module)
{
    if (module == IOT_RX_I2S_MODULE) {
        i2s_config_rx_start();
    } else if (module == IOT_TX_I2S_MODULE) {
        i2s_config_tx_start();
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_stop(IOT_I2S_MODULE module)
{
    if (module == IOT_RX_I2S_MODULE) {
        i2s_config_rx_stop();
    } else if (module == IOT_TX_I2S_MODULE) {
        i2s_config_tx_stop();
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_mclk_enable(IOT_I2S_MODULE module)
{
    if (module == IOT_RX_I2S_MODULE) {
        audio_enable_module(AUDIO_MODULE_MCLK_RX);
    } else if (module == IOT_TX_I2S_MODULE) {
        audio_enable_module(AUDIO_MODULE_MCLK_TX);
    } else {
        return RET_INVAL;
    }

    return RET_OK;
}

uint8_t iot_i2s_mclk_disable(IOT_I2S_MODULE module)
{
    if (module == IOT_RX_I2S_MODULE) {
        audio_disable_module(AUDIO_MODULE_MCLK_RX);
    } else if (module == IOT_TX_I2S_MODULE) {
        audio_disable_module(AUDIO_MODULE_MCLK_TX);
    } else {
        return RET_INVAL;
    }
    return RET_OK;
}

uint8_t iot_i2s_mclk_set_gpio(IOT_I2S_MODULE module, IOT_I2S_MCLK mclk, uint8_t gpio)
{
    uint16_t pin[1];

    pin[0] = (uint16_t)gpio;
    if (gpio_claim_group(pin, 1, true) != RET_OK) {
        return RET_INVAL;
    }

    switch(mclk)
    {
        case IOT_I2S_MCLK_16MHZ:
            if (module == IOT_RX_I2S_MODULE) {
                gpio_mtx_set_out_signal(gpio, GPIO_MTX_MCLK_OUT);
            } else if (module == IOT_TX_I2S_MODULE) {
                gpio_mtx_set_out_signal(gpio, GPIO_MTX_MCLK_OUT);
            } else {
                return RET_INVAL;
            }
            break;
        case IOT_I2S_MCLK_24MHZ:
            if (module == IOT_RX_I2S_MODULE) {
                gpio_mtx_set_out_signal(gpio, GPIO_MTX_IIS_MCLK0_OUT);
            } else if (module == IOT_TX_I2S_MODULE) {
                gpio_mtx_set_out_signal(gpio, GPIO_MTX_IIS_MCLK0_OUT);
            } else {
                return RET_INVAL;
            }
            break;
        case IOT_I2S_MCLK_32MHZ:
            if (module == IOT_RX_I2S_MODULE) {
                gpio_mtx_set_out_signal(gpio, GPIO_MTX_IIS_MCLK0_OUT);
            } else if (module == IOT_TX_I2S_MODULE) {
                gpio_mtx_set_out_signal(gpio, GPIO_MTX_IIS_MCLK0_OUT);
            } else {
                return RET_INVAL;
            }
            break;
        case IOT_I2S_MCLK_48MHZ:
            if (module == IOT_RX_I2S_MODULE) {
                gpio_mtx_set_out_signal(gpio, GPIO_MTX_IIS_MCLK1_OUT);
            } else if (module == IOT_TX_I2S_MODULE) {
                gpio_mtx_set_out_signal(gpio, GPIO_MTX_IIS_MCLK1_OUT);
            } else {
                return RET_INVAL;
            }
            break;
        default:
            break;
    }

    return RET_OK;
}
