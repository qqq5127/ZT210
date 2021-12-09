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
//#include "stdio.h"
//#include "riscv_cpu.h"

/* hal includes */
#include "iot_pdm.h"

/* hw includes */
//#include "apb.h"
//#include "aud_if.h"
#include "aud_glb.h"
#include "sdm_adc.h"
#include "gpio.h"
#include "gpio_mtx.h"
#include "aud_intf_pwr.h"

/*lint -esym(754,iot_pdm_info::used) */
typedef struct iot_pdm_info {
    bool_t used;
    bool_t slave_mode;
} iot_pdm_info_t;

static iot_pdm_info_t pdm_info[IOT_PDM_PORT_MAX];

void iot_pdm_enable(IOT_PDM_PORT port)
{
    //audio intf power on vote
    aud_intf_pwr_on(AUDIO_MODULE_PDM_RX_0 + port);
    audio_enable_module(AUDIO_MODULE_PDM_RX_0 + (AUDIO_MODULE_ID)port);
}

void iot_pdm_disable(IOT_PDM_PORT port)
{
    audio_disable_module(AUDIO_MODULE_PDM_RX_0 + (AUDIO_MODULE_ID)port);
    //audio intf power off vote
    aud_intf_pwr_off(AUDIO_MODULE_PDM_RX_0 + port);
}

void iot_pdm_start(IOT_PDM_PORT port)
{
    sdm_adc_pdm_enable((PDM_CHN_ID)port, true);
}

void iot_pdm_stop(IOT_PDM_PORT port)
{
    sdm_adc_pdm_enable((PDM_CHN_ID)port, false);
}

void iot_pdm_reset(IOT_PDM_PORT port)
{
    audio_reset_module(AUDIO_MODULE_PDM_RX_0 + (AUDIO_MODULE_ID)port);
    sdm_adc_pdm_reset((PDM_CHN_ID)port);
}

uint8_t iot_pdm_config(IOT_PDM_PORT port)
{
    sdm_adc_pdm_config_t pdm_cfg;

    pdm_cfg.pdm_slave = false;
    pdm_cfg.pdm_phase = 0;
    pdm_cfg.pdm_bck_div = 4;
    if (port >= IOT_PDM_PORT_MAX) {
        return RET_INVAL;
    }

    pdm_info[port].slave_mode = false;
    sdm_adc_pdm_config((PDM_CHN_ID)port, &pdm_cfg);

    return RET_OK;
}

uint8_t iot_pdm_pin_config(IOT_PDM_PORT port, const iot_pdm_gpio_cfg_t *gpio_cfg)
{
    uint16_t pin[2];
    pin[0] = gpio_cfg->clk;
    pin[1] = gpio_cfg->sd;

    if (gpio_claim_group(pin, 2, true) != RET_OK) {
        return RET_INVAL;
    }

    if (pdm_info[port].slave_mode) {
        gpio_mtx_set_in_signal(gpio_cfg->clk,
                               GPIO_MTX_PDM0_RX_CLK_IN + (GPIO_MTX_SIGNAL_IN)(port * 2),
                               GPIO_MTX_MODE_MATRIX);
    } else {
        gpio_mtx_set_out_signal(gpio_cfg->clk,
                                GPIO_MTX_PDM0_RX_CLK_OUT + (GPIO_MTX_SIGNAL_OUT)port);
    }
    gpio_mtx_set_in_signal(gpio_cfg->sd, GPIO_MTX_PDM0_RX_SD_IN + (GPIO_MTX_SIGNAL_IN)(port * 2),
                           GPIO_MTX_MODE_MATRIX);

    return RET_OK;
}
