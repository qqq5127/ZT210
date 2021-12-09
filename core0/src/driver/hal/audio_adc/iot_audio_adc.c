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
//common
#include "types.h"
#include "string.h"
#include "aud_glb.h"
//hal
#include "iot_mic_adc.h"
#include "iot_audio_adc.h"
#include "aud_intf_pwr.h"

#include "driver_dbglog.h"

#define IOT_AUDIO_ADC_FORMAT        4
#define IOT_AUDIO_ADC_LINK_CHN_INIT 3

typedef struct iot_audio_adc_info {
    int8_t adc_counter[IOT_AUDIO_ADC_PORT_MAX];
} iot_audio_adc_info_t;
static iot_audio_adc_info_t iot_audio_adc;

uint8_t iot_audio_adc_start(IOT_AUDIO_ADC_PORT_ID port, IOT_AUDIO_CHN_ID chn,
                                const iot_audio_adc_config_t *cfg)
{
    IOT_RX_DFE_CHN_ID def_chn = (IOT_RX_DFE_CHN_ID)chn;
    if (iot_rx_dfe_claim_channel(def_chn) != RET_OK) {
        return RET_FAIL;
    }

    iot_rx_dfe_reset(def_chn);
    iot_rx_dfe_enable(def_chn);
    iot_rx_dfe_set_adc_format((IOT_RX_DFE_ADC_ID)port, IOT_AUDIO_ADC_FORMAT);
    iot_rx_dfe_config(def_chn, &cfg->dfe);
    iot_rx_dfe_link_adc(def_chn, port);
    iot_rx_dfe_start(def_chn);

    return RET_OK;
}

uint8_t iot_audio_adc_stop(IOT_AUDIO_ADC_PORT_ID port, IOT_AUDIO_CHN_ID chn)
{
    UNUSED(port);
    IOT_RX_DFE_CHN_ID def_chn = (IOT_RX_DFE_CHN_ID)chn;

    if (iot_rx_dfe_release_channel(def_chn) != RET_OK) {
        return RET_FAIL;
    }

    iot_rx_dfe_stop(def_chn);
    iot_rx_dfe_disable(def_chn);
    iot_rx_dfe_link_adc(def_chn, IOT_AUDIO_ADC_LINK_CHN_INIT);

    return RET_OK;
}

void iot_audio_adc_multipath_sync(void)
{
    audio_multipath_enable();
}

void iot_audio_adc_init(void)
{
    iot_audio_adc.adc_counter[IOT_AUDIO_ADC_PORT_0] = 0;
    iot_audio_adc.adc_counter[IOT_AUDIO_ADC_PORT_1] = 0;
    iot_audio_adc.adc_counter[IOT_AUDIO_ADC_PORT_2] = 0;
    iot_rx_dfe_init();
    iot_mic_adc_init();
}

void iot_audio_adc_deinit(void)
{
    iot_rx_dfe_deinit();
    memset(&iot_audio_adc, 0x0, sizeof(iot_audio_adc_info_t));
}

uint8_t iot_audio_adc_open(uint8_t adc_bitmap, uint8_t power_bitmap, iot_audio_adc_timer_done_callback cb)
{
    assert(adc_bitmap != 0);
    uint8_t port_mask = 0;

    DBGLOG_DRIVER_INFO("[AUDIO ADC] adc open, adc_bitmap:0x%x, power_bitmap;0x%x", adc_bitmap, power_bitmap);
    aud_intf_pwr_on(AUDIO_MODULE_MCLK_ADC);//audio intf power on vote
    if ((iot_audio_adc.adc_counter[IOT_AUDIO_ADC_PORT_0] == 0) &&
        (iot_audio_adc.adc_counter[IOT_AUDIO_ADC_PORT_1] == 0) &&
        (iot_audio_adc.adc_counter[IOT_AUDIO_ADC_PORT_2] == 0)) {
        iot_sdm_adc_mclk_enable();
        iot_mic_reg_init();
    }

    for(uint8_t port_id = 0; port_id < IOT_AUDIO_ADC_PORT_MAX; port_id++) {
        if(adc_bitmap & BIT(port_id)) {
            assert(iot_audio_adc.adc_counter[port_id] < IOT_AUDIO_ADC_PORT_MAX);

            if(!iot_audio_adc.adc_counter[port_id]) {
                port_mask |= (uint8_t)BIT(port_id);
            }

            iot_audio_adc.adc_counter[port_id]++;
        }
    }
    iot_mic_bias_switch_on(port_mask, power_bitmap);
    return iot_mic_config(port_mask, (iot_mic_adc_timer_done_callback)cb);
}

uint8_t iot_audio_adc_close(uint8_t adc_bitmap)
{
    assert(adc_bitmap != 0);
    uint8_t ret;
    uint8_t port_mask = 0;

    DBGLOG_DRIVER_INFO("[AUDIO ADC] adc close, adc_bitmap:0x%x", adc_bitmap);

    for(uint8_t port_id = 0; port_id < IOT_AUDIO_ADC_PORT_MAX; port_id++) {
        if(adc_bitmap & BIT(port_id)) {
            assert(iot_audio_adc.adc_counter[port_id] > 0);
            iot_audio_adc.adc_counter[port_id]--;

            if(!iot_audio_adc.adc_counter[port_id]) {
                port_mask |= (uint8_t)BIT(port_id);
            }
        }
    }

    iot_mic_release(port_mask);
    ret = iot_mic_bias_switch_off(port_mask);

    if ((iot_audio_adc.adc_counter[IOT_AUDIO_ADC_PORT_0] == 0) &&
        (iot_audio_adc.adc_counter[IOT_AUDIO_ADC_PORT_1] == 0) &&
        (iot_audio_adc.adc_counter[IOT_AUDIO_ADC_PORT_2] == 0)) {
        iot_sdm_adc_mclk_disable();
        iot_mic_bias_deinit();
        aud_intf_pwr_off(AUDIO_MODULE_MCLK_ADC);//audio intf power off vote
    }

    return ret;
}

uint8_t iot_audio_adc_gain_set(IOT_AUDIO_CHN_ID chn, int16_t gain)
{
    if (chn >= IOT_AUDIO_CHN_MAX) {
        return RET_INVAL;
    }
    iot_rx_dfe_gain_set((IOT_RX_DFE_CHN_ID)chn, gain);

    return RET_OK;
}
