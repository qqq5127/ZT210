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
#include "string.h"
#include "math.h"
#include "chip_irq_vector.h"

/* hw includes */
#include "apb.h"
#include "aud_glb.h"
#include "aud_if.h"
#include "asrc.h"
#include "dma.h"
#include "aud_intf_pwr.h"
#include "iot_irq.h"
#include "iot_audio.h"
#include "iot_asrc.h"

#include "driver_dbglog.h"

#define ASRC_PPM_UNIT          0.01
#define ASRC_MIN_PPM_VALUE     -1000
#define ASRC_MAX_PPM_VALUE     1000
#define ASRC_INPUT_RATE        1000000
#define ASRC_FARROW_INPUT_FREQ 2000000
#define ASRC_TX_BIQUAD_EN      0x03
#define ASRC_RX_BIQUAD_EN      0x03
#define ASRC_RX_FREQUENT_DIV   0x00
#define ASRC_FILTER_NUM_MAX    0x07
#define ASRC_SAMPLE_FREQUENCE  16000000
/* ((1 << 26) */
#define ASRC_RX_FARROW_DELTA_PPM_TEMP_VALUE 67108864
/* ((1 << 31) / sysclock) */
#define ASRC_TX_FARROW_DELTA_PPM_TEMP_VALUE_DOUBLU 134.217728000000
#define ASRC_COEFF_PARAM_MAX                       10

typedef struct asrc_chn_info {
    bool_t used;
    bool_t tx_mode;
    bool_t use_timer;
    uint8_t rx_src;
    uint8_t tx_dma_chan;
    uint8_t filter_num;
} iot_asrc_chn_info_t;

static iot_asrc_chn_info_t asrc_info[IOT_ASRC_CHANNEL_MAX];
static const uint32_t iot_asrc_int_vector[IOT_ASRC_CHANNEL_TX_MAX] = {
    ASRC0_INT,
    ASRC1_INT,
};
bool_t asrc_timer_start_all = false;
static iot_irq_t iot_asrc_interrupt_irq[IOT_ASRC_CHANNEL_TX_MAX];
static iot_asrc_int_hook iot_asrc_under_run_cb[IOT_ASRC_CHANNEL_TX_MAX];
static iot_asrc_int_hook iot_asrc_reach_cnt_cb[IOT_ASRC_CHANNEL_TX_MAX];

/* mode: 0: ceil log2; 1: floor */
static uint32_t iot_asrc_log2(uint32_t value, uint8_t mode)
{
    uint32_t ret;
    uint32_t temp = value;

    if (!mode) {
        temp = (temp * 2) - 1;
        ret = log2(temp);
    } else {
        ret = log2(temp);
    }
    return ret;
}

void iot_asrc_enable(IOT_ASRC_CHANNEL_ID id, bool_t sync)
{
    if (sync) {
        audio_cfg_multipath((AUDIO_MODULE_ID)(AUDIO_MODULE_ASRC_0 + id));
    } else {
        audio_enable_module((AUDIO_MODULE_ID)(AUDIO_MODULE_ASRC_0 + id));
    }
}

void iot_asrc_disable(IOT_ASRC_CHANNEL_ID id, bool_t sync)
{
    if (sync) {
        audio_release_multipath((AUDIO_MODULE_ID)(AUDIO_MODULE_ASRC_0 + id));
    } else {
        audio_disable_module((AUDIO_MODULE_ID)(AUDIO_MODULE_ASRC_0 + id));
    }
}

void iot_asrc_reset(IOT_ASRC_CHANNEL_ID id)
{
    audio_reset_module((AUDIO_MODULE_ID)(AUDIO_MODULE_ASRC_0 + id));
}

void iot_asrc_start(IOT_ASRC_CHANNEL_ID id)
{
    asrc_enable((ASRC_CHANNEL_ID)id, true);
}

void iot_asrc_stop(uint8_t bitmap_id)
{
    for (uint8_t id = 0; id < IOT_ASRC_CHANNEL_MAX; id++) {
        if (BIT(id) & bitmap_id) {
            if (!asrc_info[id].used) {
                DBGLOG_DRIVER_WARNING("iot_asrc_stop bitmap_id:%x id:%d\n", bitmap_id, id);
                assert(0);
            } else {
                if (id < IOT_ASRC_CHANNEL_TX_MAX && asrc_info[id].tx_mode) {
                    iot_dma_chs_flush(BIT(asrc_info[id].tx_dma_chan));
                }

                asrc_enable((ASRC_CHANNEL_ID)id, false);

                if (id < IOT_ASRC_CHANNEL_TX_MAX && asrc_info[id].tx_mode) {
                    iot_dma_channel_suspend((IOT_DMA_CHANNEL_ID)asrc_info[id].tx_dma_chan);
                    iot_dma_channel_reset((IOT_DMA_CHANNEL_ID)asrc_info[id].tx_dma_chan);
                    dma_release_channel(DMA_CONTROLLER0, (DMA_CHANNEL_ID)asrc_info[id].tx_dma_chan);
                }
            }
        }
    }
}

uint8_t iot_tx_asrc_from_mem_mount_dma(IOT_ASRC_CHANNEL_ID id, const char *src, uint32_t size,
                                       dma_mem_peri_done_callback cb)
{
    uint8_t ret;

    if (id >= IOT_ASRC_CHANNEL_TX_MAX) {
        return RET_INVAL;
    }

    if (!asrc_info[id].used) {
        return RET_INVAL;
    }

    if (!asrc_info[id].tx_mode) {
        return RET_INVAL;
    }

    ret = iot_dma_mem_to_peri((IOT_DMA_CHANNEL_ID)asrc_info[id].tx_dma_chan,
                        (void *)(asrc_get_tx_dma_addr((ASRC_CHANNEL_ID)id)), src, size, cb);

    if (ret != RET_OK) {
        DBGLOG_DRIVER_WARNING("asrc dma mount fail: ret %d, id %d, cb %p\n", ret, id, cb);
    }

    return ret;
}

static uint8_t iot_asrc_prepare_tx_dma(IOT_ASRC_CHANNEL_ID id)
{
    IOT_DMA_CHANNEL_ID dma_ch;
    IOT_DMA_PERI_REQ req;
    uint8_t ret;

    assert(asrc_info[id].tx_dma_chan == IOT_DMA_CHANNEL_NONE);
    ret = iot_dma_claim_channel(&dma_ch);
    if (ret != RET_OK) {
        return ret;
    }

    req = (IOT_DMA_PERI_REQ)(IOT_DMA_PERI_ASRC0 + id);
    ret = iot_dma_ch_peri_config(dma_ch, IOT_DMA_TRANS_MEM_TO_PERI, IOT_DMA_DATA_WIDTH_WORD,
                                 IOT_DMA_CH_PRIORITY_HIGH, IOT_DMA_PERI_REQ_NONE, req);
    if (ret != RET_OK) {
        iot_dma_free_channel(dma_ch);
        return ret;
    }

    asrc_info[id].tx_dma_chan = dma_ch;

    return RET_OK;
}

uint8_t iot_asrc_set_tx_mode(IOT_ASRC_CHANNEL_ID id)
{
    uint8_t ret;

    if (id >= IOT_ASRC_CHANNEL_TX_MAX) {
        return RET_INVAL;
    }

    ret = iot_asrc_prepare_tx_dma(id);
    if (ret != RET_OK) {
        return ret;
    }

    asrc_set_farrow_mode((ASRC_CHANNEL_ID)id, ASRC_FARROW_TX);
    asrc_out_select((ASRC_CHANNEL_ID)id, ASRC_OUT_SEL_TO_DAC);

    asrc_info[id].tx_mode = true;

    return RET_OK;
}

uint8_t iot_asrc_set_rx_mode(IOT_ASRC_CHANNEL_ID id)
{
    if (id >= IOT_ASRC_CHANNEL_MAX) {
        return RET_INVAL;
    }

    asrc_set_farrow_mode((ASRC_CHANNEL_ID)id, ASRC_FARROW_RX);
    asrc_out_select((ASRC_CHANNEL_ID)id, ASRC_OUT_SEL_TO_MEMORY);   // TODO: need this ??

    asrc_info[id].tx_mode = false;

    return RET_OK;
}

uint8_t iot_asrc_link_rx_dfe(IOT_ASRC_CHANNEL_ID id, uint8_t chan)
{
    if (id >= IOT_ASRC_CHANNEL_MAX) {
        return RET_INVAL;
    }

    if (chan >= AUDIO_RX_DFE_MAX) {
        return RET_INVAL;
    }

    if (!asrc_info[id].used) {
        return RET_INVAL;
    }

    if (asrc_info[id].tx_mode) {
        return RET_INVAL;
    }

    audio_set_rx_asrc_src((AUDIO_RX_ASRC_ID)id,
                          (AUDIO_RX_ASRC_SRC_ID)(AUDIO_RX_ASRC_SRC_DFE_0 + chan));
    asrc_info[id].rx_src = AUDIO_RX_ASRC_SRC_DFE_0 + chan;

    return RET_OK;
}

uint8_t iot_asrc_link_rx_i2s(IOT_ASRC_CHANNEL_ID id, uint8_t chan)
{
    if (id >= IOT_ASRC_CHANNEL_MAX) {
        return RET_INVAL;
    }

    if (chan >= 8) {   // TODO : fix me
        return RET_INVAL;
    }

    if (!asrc_info[id].used) {
        return RET_INVAL;
    }

    if (asrc_info[id].tx_mode) {
        return RET_INVAL;
    }

    audio_set_rx_asrc_src((AUDIO_RX_ASRC_ID)id,
                          (AUDIO_RX_ASRC_SRC_ID)(AUDIO_RX_ASRC_SRC_I2S_0 + chan));
    asrc_info[id].rx_src = AUDIO_RX_ASRC_SRC_I2S_0 + chan;

    return RET_OK;
}

uint8_t iot_asrc_clear_trans_mode(IOT_ASRC_CHANNEL_ID id)
{
    if (id >= IOT_ASRC_CHANNEL_MAX) {
        return RET_INVAL;
    }

    if (!asrc_info[id].used) {
        return RET_OK;
    }

    if ((IOT_DMA_PERI_REQ)(asrc_info[id].tx_dma_chan) != IOT_DMA_PERI_REQ_NONE) {
        iot_dma_free_channel((IOT_DMA_CHANNEL_ID)(asrc_info[id].tx_dma_chan));
        asrc_info[id].tx_dma_chan = IOT_DMA_PERI_REQ_NONE;
    }

    asrc_info[id].used = false;

    return RET_OK;
}

uint8_t iot_asrc_tx_config_frequence(IOT_ASRC_CHANNEL_ID id, uint32_t freq_in, uint32_t freq_out,
                                     int32_t ppm)
{
    uint32_t temp0;
    uint32_t temp1;
    uint32_t sysclock;
    uint32_t hbf_stage_num;
    double freq_in_real;
    double freq_in_max;
    double freq_in_min;
    double double_temp0;
    double double_temp1;
    asrc_frequence_config_t asrc_tx_cfg;

    sysclock = ASRC_SAMPLE_FREQUENCE;
    asrc_tx_cfg.fout_div = (uint16_t)(sysclock / freq_out);
    asrc_tx_cfg.biquad_en = ASRC_TX_BIQUAD_EN;

    freq_in_max = freq_in / (1 + ((double)(ASRC_MIN_PPM_VALUE) / (double)ASRC_INPUT_RATE));

    freq_in_min = freq_in / (1 + ((double)(ASRC_MAX_PPM_VALUE) / (double)ASRC_INPUT_RATE));

    if (freq_out >= ((uint32_t)freq_in_min << ASRC_FILTER_NUM_MAX)) {
        temp0 = ASRC_FILTER_NUM_MAX;
    } else {
        temp1 = freq_out / (uint32_t)freq_in_min;
        temp0 = (uint32_t)iot_asrc_log2(temp1, 0);
    }

    if (ASRC_FARROW_INPUT_FREQ >= ((uint32_t)freq_in_max << ASRC_FILTER_NUM_MAX)) {
        temp1 = ASRC_FILTER_NUM_MAX;
    } else {
        temp0 = ASRC_FARROW_INPUT_FREQ / (uint32_t)freq_in_max;
        temp1 = (uint32_t)iot_asrc_log2(temp0, 1);
    }

    asrc_info[id].filter_num = (uint8_t)CLAMP(temp0, (uint32_t)5, temp1);

    if (asrc_info[id].filter_num <= 5) {
        asrc_tx_cfg.cic_en = false;
        hbf_stage_num = asrc_info[id].filter_num;
    } else if (asrc_info[id].filter_num > 5 && asrc_info[id].filter_num <= ASRC_FILTER_NUM_MAX) {
        asrc_tx_cfg.cic_en = true;
        hbf_stage_num = asrc_info[id].filter_num - 2;
    } else {
        /*filter number must not more than 7 */
        return RET_INVAL;
    }
    asrc_tx_cfg.hb_filter_en = (uint8_t)((1U << hbf_stage_num) - 1);

    freq_in_real = freq_in / (1 + ((double)(ppm * ASRC_PPM_UNIT) / (double)ASRC_INPUT_RATE));

    double_temp0 =
        (double)(1U << asrc_info[id]
                           .filter_num);   //lint !e790 Suspicious truncation, integral to float
    double_temp0 = (freq_in_real * double_temp0);
    double_temp1 = ASRC_TX_FARROW_DELTA_PPM_TEMP_VALUE_DOUBLU;
    asrc_tx_cfg.farrow_delta_w_ppm = (uint32_t)(double_temp0 * double_temp1);

    asrc_frequence_config((ASRC_CHANNEL_ID)id, &asrc_tx_cfg);
    return RET_OK;
}

uint8_t iot_asrc_get_upsample(IOT_ASRC_CHANNEL_ID id)
{
    return (uint8_t)BIT(asrc_info[id].filter_num);
}

uint8_t iot_asrc_tx_ppm_set(IOT_ASRC_CHANNEL_ID id, uint32_t freq_in, int32_t ppm)
{
    double freq_in_real;
    double double_temp0;
    double double_temp1;
    asrc_frequence_config_t asrc_tx_cfg;

    freq_in_real = freq_in / (1 + ((double)(ppm * ASRC_PPM_UNIT) / (double)ASRC_INPUT_RATE));

    double_temp0 =
        (double)(1U << asrc_info[id]
                           .filter_num);   //lint !e790 Suspicious truncation, integral to float
    double_temp0 = (freq_in_real * double_temp0);
    double_temp1 = ASRC_TX_FARROW_DELTA_PPM_TEMP_VALUE_DOUBLU;
    asrc_tx_cfg.farrow_delta_w_ppm = (uint32_t)(double_temp0 * double_temp1);

    asrc_ppm_set((ASRC_CHANNEL_ID)id, asrc_tx_cfg.farrow_delta_w_ppm);
    return RET_OK;
}

uint8_t iot_asrc_tx_filter_ppm_set(IOT_ASRC_CHANNEL_ID id, uint32_t freq_in, int32_t ppm,
                                   uint8_t filter_num)
{
    double freq_in_real;
    double double_temp0;
    double double_temp1;
    asrc_frequence_config_t asrc_tx_cfg;

    freq_in_real = freq_in / (1 + ((double)(ppm * ASRC_PPM_UNIT) / (double)ASRC_INPUT_RATE));

    double_temp0 =
        (double)(1U << filter_num);   //lint !e790 Suspicious truncation, integral to float
    double_temp0 = (freq_in_real * double_temp0);
    double_temp1 = ASRC_TX_FARROW_DELTA_PPM_TEMP_VALUE_DOUBLU;
    asrc_tx_cfg.farrow_delta_w_ppm = (uint32_t)(double_temp0 * double_temp1);

    asrc_ppm_set((ASRC_CHANNEL_ID)id, asrc_tx_cfg.farrow_delta_w_ppm);
    return RET_OK;
}

uint8_t iot_asrc_over_sample_factor_get(uint32_t freq_in, uint32_t freq_out)
{
    uint8_t spk_over_sample = 0;
    assert(freq_out == 800000u);   //out is 800k, others TODO: add
    switch (freq_in) {
        case 8000:
            spk_over_sample = 128;
            break;
        case 16000:
            spk_over_sample = 64;
            break;
        case 32000:
        case 44100:
        case 48000:
            spk_over_sample = 32;
            break;
        case 96000:
            spk_over_sample = 16;
            break;
        case 192000:
            spk_over_sample = 8;
            break;
        default:
            assert(0);
            break;
    }
    return spk_over_sample;
}

uint8_t iot_asrc_over_filter_num_get(uint32_t freq_in, uint32_t freq_out)
{
    uint8_t filter_num = 0;
    assert(freq_out == 800000u);   //out is 800k, others TODO: add
    switch (freq_in) {
        case 8000:
            filter_num = 7;
            break;
        case 16000:
            filter_num = 6;
            break;
        case 32000:
        case 44100:
        case 48000:
            filter_num = 5;
            break;
        case 96000:
            filter_num = 4;
            break;
        case 192000:
            filter_num = 3;
            break;
        default:
            assert(0);
            break;
    }
    return filter_num;
}

void iot_asrc_coeff_config(IOT_ASRC_CHANNEL_ID id, const uint32_t *coeff)
{
    asrc_coeff_config((ASRC_CHANNEL_ID)id, coeff);
}

void iot_asrc_tx_coeff_config(IOT_ASRC_CHANNEL_ID id, IOT_ASRC_COEFF_PARAM_ID num, uint32_t value)
{
    asrc_coeff_param_set((ASRC_CHANNEL_ID)id, (ASRC_COEFF_PARAM_ID)num, value);
}

uint8_t iot_asrc_rx_config_frequence(IOT_ASRC_CHANNEL_ID id, uint32_t freq_in, uint32_t freq_out,
                                     int32_t ppm)
{
    if (id >= IOT_ASRC_CHANNEL_MAX) {
        return RET_INVAL;
    }

    uint32_t log2_temp;
    double freq_out_real;
    double freq_out_min;
    double double_temp0;
    double double_temp1;
    asrc_frequence_config_t asrc_rx_cfg;

    asrc_rx_cfg.fout_div = ASRC_RX_FREQUENT_DIV;
    asrc_rx_cfg.biquad_en = ASRC_RX_BIQUAD_EN;
    asrc_rx_cfg.cic_en = false;

    freq_out_min = freq_out * (1 + ((double)(ASRC_MIN_PPM_VALUE) / (double)ASRC_INPUT_RATE));

    if ((64 * freq_out_min - 1) >= (freq_in << ASRC_FILTER_NUM_MAX))   //lint !e790
    {
        double_temp0 = ASRC_FILTER_NUM_MAX;
    } else {
        log2_temp = (uint32_t)(64 * freq_out_min - 1) / freq_in;
        double_temp0 = (double)iot_asrc_log2(log2_temp, 1);
    }
    asrc_info[id].filter_num = MIN((uint8_t)(double_temp0), (uint8_t)5);

    if (ASRC_FARROW_INPUT_FREQ >= (freq_in << ASRC_FILTER_NUM_MAX)) {
        double_temp1 = ASRC_FILTER_NUM_MAX;
    } else {
        log2_temp = ASRC_FARROW_INPUT_FREQ / freq_in;
        double_temp1 = (double)iot_asrc_log2(log2_temp, 1);
    }
    asrc_info[id].filter_num = MIN((uint8_t)(double_temp1), asrc_info[id].filter_num);
    asrc_rx_cfg.hb_filter_en = (uint8_t)((0x01U << asrc_info[id].filter_num) - 1);

    freq_out_real = freq_out * (1 + ((double)ppm * ASRC_PPM_UNIT) / (double)ASRC_INPUT_RATE);

    double_temp0 =
        (freq_in
         << asrc_info[id].filter_num);   //lint !e790 Suspicious truncation, integral to float
    double_temp1 = (ASRC_RX_FARROW_DELTA_PPM_TEMP_VALUE / freq_out_real);
    asrc_rx_cfg.farrow_delta_w_ppm = (uint32_t)(double_temp0 * double_temp1);

    asrc_frequence_config((ASRC_CHANNEL_ID)id, &asrc_rx_cfg);
    return RET_OK;
}

uint8_t iot_asrc_rx_filter_ppm_set(IOT_ASRC_CHANNEL_ID id, uint32_t freq_out, int32_t ppm,
                                        uint8_t filter_num)
{
    if (id >= IOT_ASRC_CHANNEL_MAX) {
        assert(0);
    }

    asrc_frequence_config_t asrc_rx_cfg;
    double freq_out_real, double_temp0, double_temp1;

    freq_out_real = freq_out * (1 + ((double)ppm * ASRC_PPM_UNIT) / (double)ASRC_INPUT_RATE);

    double_temp0 =
        (freq_out << filter_num);   //lint !e790 Suspicious truncation, integral to float
    double_temp1 = (ASRC_RX_FARROW_DELTA_PPM_TEMP_VALUE / freq_out_real);
    asrc_rx_cfg.farrow_delta_w_ppm = (uint32_t)(double_temp0 * double_temp1);

    asrc_ppm_set((ASRC_CHANNEL_ID)id, asrc_rx_cfg.farrow_delta_w_ppm);

    return RET_OK;
}

uint8_t iot_asrc_rx_ppm_set(IOT_ASRC_CHANNEL_ID id, uint32_t freq_in, uint32_t freq_out,
                            int32_t ppm)
{
    asrc_frequence_config_t asrc_rx_cfg;
    double freq_out_real, double_temp0, double_temp1;

    freq_out_real = freq_out * (1 + ((double)ppm * ASRC_PPM_UNIT) / (double)ASRC_INPUT_RATE);

    double_temp0 =
        (freq_in
         << asrc_info[id].filter_num);   //lint !e790 Suspicious truncation, integral to float
    double_temp1 = (ASRC_RX_FARROW_DELTA_PPM_TEMP_VALUE / freq_out_real);
    asrc_rx_cfg.farrow_delta_w_ppm = (uint32_t)(double_temp0 * double_temp1);

    asrc_ppm_set((ASRC_CHANNEL_ID)id, asrc_rx_cfg.farrow_delta_w_ppm);
    return RET_OK;
}

uint8_t iot_asrc_set_half_word_mode(IOT_ASRC_CHANNEL_ID id, IOT_ASRC_HALF_WORD_MODE mode)
{
    if (id >= IOT_ASRC_CHANNEL_MAX) {
        return RET_INVAL;
    }

    if (mode >= IOT_ASRC_HALF_WORD_MODE_MAX) {
        return RET_INVAL;
    }

    asrc_set_half_word_mode((ASRC_CHANNEL_ID)id, (ASRC_HALF_WORD_MODE)mode);
    return RET_OK;
}

uint8_t iot_asrc_config_start_src(IOT_ASRC_CHANNEL_ID id, IOT_ASRC_START_SRC src)
{
    if (id >= IOT_ASRC_CHANNEL_MAX) {
        return RET_INVAL;
    }

    switch (src) {
        case IOT_ASRC_SELF_START:
            asrc_self_start((ASRC_CHANNEL_ID)id, true);
            audio_set_asrc_hw_start_src(id, AUDIO_ASRC_START_NO_HW_SRC);
            break;
        case IOT_ASRC_BT_TRIGGER:
            asrc_self_start((ASRC_CHANNEL_ID)id, false);
            audio_set_asrc_hw_start_src(id, AUDIO_ASRC_START_BY_BT);
            break;
        case IOT_ASRC_TIMER_TRIGGER:
            asrc_self_start((ASRC_CHANNEL_ID)id, false);
            audio_set_asrc_hw_start_src(id, AUDIO_ASRC_START_BY_TIMER);
            break;
        case IOT_ASRC_START_SRC_MAX:
            break;
        default:
            return RET_INVAL;
    }

    return RET_OK;
}

uint8_t iot_asrc_config_latch_src(IOT_ASRC_CHANNEL_ID id, IOT_ASRC_LATCH_SRC src)
{
    if (id >= IOT_ASRC_CHANNEL_MAX) {
        return RET_INVAL;
    }

    switch (src) {
        case IOT_ASRC_BT_LATCH:
            audio_set_asrc_hw_latch_src(id, AUDIO_ASRC_LATCH_BY_BT);
            break;
        case IOT_ASRC_TIMER_LATCH:
            audio_set_asrc_hw_latch_src(id, AUDIO_ASRC_LATCH_BY_TIMER);
            break;
        case IOT_ASRC_TIMER_LATCH_NONE:
            break;
        case IOT_ASRC_LATCH_SRC_MAX:
            break;
        default:
            return RET_INVAL;
    }

    return RET_OK;
}

uint8_t iot_asrc_overwrite_start(uint8_t start)
{
    if (start >= 4) {
        return RET_INVAL;
    }
    audio_set_bt_start_asrc_overwrite(start);
    return RET_OK;
}

uint8_t iot_asrc_overwrite_latch(uint8_t latch)
{
    if (latch >= 4) {
        return RET_INVAL;
    }
    audio_set_bt_latch_overwrite(latch);
    return RET_OK;
}

void iot_asrc_enable_bt_tgt(bool_t enable)
{
    audio_enable_asrc_bt_tgt(enable);
}

void iot_asrc_set_target_sample_cnt(IOT_ASRC_CHANNEL_ID id, uint32_t cnt)
{
    asrc_set_target_sample_cnt((ASRC_CHANNEL_ID)id, cnt);
}

uint32_t iot_asrc_get_latched_sample_cnt(IOT_ASRC_CHANNEL_ID id)
    IRAM_TEXT(iot_asrc_get_latched_sample_cnt);
uint32_t iot_asrc_get_latched_sample_cnt(IOT_ASRC_CHANNEL_ID id)
{
    return asrc_get_latched_sample_cnt((ASRC_CHANNEL_ID)id);
}

uint32_t iot_asrc_get_freerun_sample_cnt(IOT_ASRC_CHANNEL_ID id)
    IRAM_TEXT(iot_asrc_get_freerun_sample_cnt);
uint32_t iot_asrc_get_freerun_sample_cnt(IOT_ASRC_CHANNEL_ID id)
{
    return asrc_get_freerun_sample_cnt((ASRC_CHANNEL_ID)id);
}

void iot_asrc_register_underrun_irq_hook(IOT_ASRC_CHANNEL_ID id, iot_asrc_int_hook hook)
{
    if (id >= IOT_ASRC_CHANNEL_TX_MAX) {
        return;
    }

    iot_asrc_under_run_cb[id] = hook;

    if (hook) {
        asrc_int_clear((ASRC_CHANNEL_ID)id, ASRC_INT_UNDER_FLOW);
        asrc_int_enable((ASRC_CHANNEL_ID)id, ASRC_INT_UNDER_FLOW);
    } else {
        asrc_int_disable((ASRC_CHANNEL_ID)id, ASRC_INT_UNDER_FLOW);
    }
}

void iot_asrc_register_reachcnt_irq_hook(IOT_ASRC_CHANNEL_ID id, iot_asrc_int_hook hook)
{
    if (id >= IOT_ASRC_CHANNEL_TX_MAX) {
        return;
    }

    iot_asrc_reach_cnt_cb[id] = hook;

    if (hook) {
        asrc_int_clear((ASRC_CHANNEL_ID)id, ASRC_INT_REACH_SAMPLE_CNT);
        asrc_int_enable((ASRC_CHANNEL_ID)id, ASRC_INT_REACH_SAMPLE_CNT);
    } else {
        asrc_int_disable((ASRC_CHANNEL_ID)id, ASRC_INT_REACH_SAMPLE_CNT);
    }
}

static uint32_t iot_asrc_isr_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_asrc_isr_handler);
static uint32_t iot_asrc_isr_handler(uint32_t vector, uint32_t data)
{
    ASRC_CHANNEL_ID asrc_id = (ASRC_CHANNEL_ID)data;
    uint32_t int_st;
    UNUSED(vector);

    int_st = asrc_int_get_status(asrc_id);
    if (int_st & BIT(ASRC_INT_UNDER_FLOW)) {
        asrc_int_clear(asrc_id, ASRC_INT_UNDER_FLOW);
        if (iot_asrc_under_run_cb[asrc_id]) {
            iot_asrc_under_run_cb[asrc_id](asrc_id);
        }
    }

    if (int_st & BIT(ASRC_INT_REACH_SAMPLE_CNT)) {
        asrc_int_clear(asrc_id, ASRC_INT_REACH_SAMPLE_CNT);
        if (iot_asrc_reach_cnt_cb[asrc_id]) {
            iot_asrc_reach_cnt_cb[asrc_id](asrc_id);
        }
    }

    uint32_t target_sample_cnt = asrc_get_target_sample_cnt(asrc_id);
    uint32_t latched_sample_cnt = asrc_get_latched_sample_cnt(asrc_id);

    uint32_t freerun_sample_cnt = asrc_get_freerun_sample_cnt(asrc_id);
    uint32_t tx_ibuf_ovt_thres = asrc_get_tx_ibuf_ovt_thres(asrc_id);
    uint32_t start_option = asrc_get_start_option(asrc_id);

    DBGLOG_DRIVER_WARNING("asrc int ongoing: %x, after: %x\n", int_st,
                          asrc_int_get_status(asrc_id));
    DBGLOG_DRIVER_WARNING("asrc tgt_cnt: %d, latch_cnt: %d, free_cnt: %d, underrun thres: %d, \
        start option: %d\n",
                          target_sample_cnt, latched_sample_cnt, freerun_sample_cnt,
                          tx_ibuf_ovt_thres, start_option);

    //Some cases do not have macro controls above -"DBGLOG_DRIVER_WARNING",like "fw_updater".
    UNUSED(target_sample_cnt);
    UNUSED(latched_sample_cnt);
    UNUSED(freerun_sample_cnt);
    UNUSED(tx_ibuf_ovt_thres);
    UNUSED(start_option);

    return RET_OK;
}

void iot_asrc_int_enable(IOT_ASRC_CHANNEL_ID id, IOT_ASRC_INT_TYPE type)
{
    asrc_int_enable((ASRC_CHANNEL_ID)id, (ASRC_INT_TYPE)type);
}

void iot_asrc_int_disable(IOT_ASRC_CHANNEL_ID id, IOT_ASRC_INT_TYPE type)
{
    asrc_int_disable((ASRC_CHANNEL_ID)id, (ASRC_INT_TYPE)type);
}

void iot_asrc_init(void)
{
    for (uint32_t i = 0; i < IOT_ASRC_CHANNEL_MAX; i++) {
        asrc_info[i].used = false;
        asrc_info[i].tx_mode = false;
        asrc_info[i].tx_dma_chan = IOT_DMA_CHANNEL_NONE;
        asrc_info[i].rx_src = AUDIO_RX_ASRC_SRC_NONE;
        asrc_info[i].filter_num = 0;
        asrc_info[i].use_timer = false;
    }
}

void iot_asrc_deinit(void)
{
    for (uint32_t i = 0; i < IOT_ASRC_CHANNEL_TX_MAX; i++) {
        iot_irq_mask(iot_asrc_interrupt_irq[i]);
        iot_irq_delete(iot_asrc_interrupt_irq[i]);
    }
    memset(asrc_info, 0x00, sizeof(iot_asrc_chn_info_t) * IOT_ASRC_CHANNEL_MAX);
}

uint8_t iot_asrc_open(IOT_ASRC_CHANNEL_ID id, const iot_asrc_config_t *cfg)
{
    if (asrc_info[id].used) {
        return RET_BUSY;
    }

    uint32_t iot_asrc_coeff_param_table[ASRC_COEFF_PARAM_MAX] = {
        0x1852, 0xfdb1, 0x24f, 0xe7ae, 0x4000, 0x1852, 0xfdb1, 0x24f, 0xe7ae, 0x4000};

    //audio intf power on vote
    aud_intf_pwr_on(AUDIO_MODULE_ASRC_0 + id);

    if (id < IOT_ASRC_CHANNEL_TX_MAX) {
        iot_asrc_under_run_cb[id] = NULL;
        iot_asrc_reach_cnt_cb[id] = NULL;

        asrc_int_disable_all((ASRC_CHANNEL_ID)id);
        asrc_int_clear_all((ASRC_CHANNEL_ID)id);

        iot_asrc_interrupt_irq[id] =
            iot_irq_create(iot_asrc_int_vector[id], id, iot_asrc_isr_handler);

        iot_irq_unmask(iot_asrc_interrupt_irq[id]);
    }

    iot_asrc_reset(id);
    iot_asrc_enable(id, cfg->sync);

    iot_asrc_config_start_src(id, cfg->trigger);
    iot_asrc_config_latch_src(id, cfg->latch);

    if (cfg->mode == IOT_ASRC_TX_MODE) {
        if (id >= IOT_ASRC_CHANNEL_TX_MAX) {
            return RET_INVAL;
        }
        if (iot_asrc_set_tx_mode(id) == RET_INVAL) {
            return RET_INVAL;
        }
        iot_asrc_tx_config_frequence(id, cfg->freq_in, cfg->freq_out, cfg->ppm);
        iot_asrc_set_half_word_mode(id, cfg->half_word);
    } else if (cfg->mode == IOT_ASRC_RX_MODE) {
        if (iot_asrc_set_rx_mode(id) == RET_INVAL) {
            return RET_INVAL;
        }
        iot_asrc_rx_config_frequence(id, cfg->freq_in, cfg->freq_out, cfg->ppm);
    }
    iot_asrc_coeff_config(id, iot_asrc_coeff_param_table);
    asrc_info[id].used = true;

    return RET_OK;
}

uint8_t iot_asrc_close(IOT_ASRC_CHANNEL_ID id)
{
    if (!asrc_info[id].used) {
        return RET_OK;
    }

    iot_asrc_disable(id, false);
    if (id < IOT_ASRC_CHANNEL_TX_MAX) {
        iot_irq_mask(iot_asrc_interrupt_irq[id]);
        iot_irq_delete(iot_asrc_interrupt_irq[id]);
    }
    asrc_info[id].used = false;
    asrc_info[id].tx_mode = false;
    asrc_info[id].tx_dma_chan = IOT_DMA_CHANNEL_NONE;
    asrc_info[id].rx_src = AUDIO_RX_ASRC_SRC_NONE;
    asrc_info[id].filter_num = 0;
    asrc_info[id].use_timer = false;

    //audio intf power off vote
    aud_intf_pwr_off(AUDIO_MODULE_ASRC_0 + id);
    return RET_OK;
}

void iot_asrc_auto_start(IOT_ASRC_CHANNEL_ID id, bool_t enable)
{
    asrc_self_start((ASRC_CHANNEL_ID)id, enable);
}

void iot_asrc_auto_stop(IOT_ASRC_CHANNEL_ID id, bool_t enable)
{
    asrc_set_auto_stop((ASRC_CHANNEL_ID)id, enable);
}

void iot_asrc_continue(IOT_ASRC_CHANNEL_ID id, bool_t enable)
{
    asrc_set_continue((ASRC_CHANNEL_ID)id, enable);
}

void iot_asrc_auto_start_threshold(IOT_ASRC_CHANNEL_ID chn, uint8_t threshold)
{
    asrc_auto_start_throshold((ASRC_CHANNEL_ID)chn, threshold);
}

uint8_t iot_asrc_timer_open(IOT_ASRC_CHANNEL_ID id, uint32_t delay_tick)
{
    if (asrc_info[id].use_timer) {
        return RET_INVAL;
    }

    uint32_t cur_tick = audio_timer_get_freerun_tick();
    audio_set_asrc_hw_start_src(id, AUDIO_ASRC_START_BY_TIMER);
    audio_timer_set_target((AUDIO_TIMER_TARGET_ID)id, delay_tick + cur_tick);
    asrc_info[id].use_timer = true;
    return RET_OK;
}

uint8_t iot_asrc_timer_close(IOT_ASRC_CHANNEL_ID id)
{
    if (!asrc_info[id].use_timer) {
        return RET_INVAL;
    }
    audio_timer_enable_target((AUDIO_TIMER_TARGET_ID)id, false);
    audio_timer_set_target((AUDIO_TIMER_TARGET_ID)id, 0);
    asrc_info[id].use_timer = false;
    return RET_OK;
}

uint8_t iot_asrc_timer_start_all(void)
{
    uint8_t mask = 0;
    if (asrc_timer_start_all) {
        return RET_BUSY;
    }
    for (int i = 0; i < IOT_ASRC_CHANNEL_MAX; i++) {
        if (asrc_info[i].use_timer) {
            mask |= (uint8_t)BIT(i);
        }
    }
    audio_timer_matrix_enable(mask);
    audio_timer_enable(true);
    asrc_timer_start_all = true;
    return RET_OK;
}

uint8_t iot_asrc_timer_stop_all(void) IRAM_TEXT(iot_asrc_timer_stop_all);
uint8_t iot_asrc_timer_stop_all(void)
{
    if (!asrc_timer_start_all) {
        return RET_BUSY;
    }
    audio_timer_enable(false);
    asrc_timer_start_all = false;
    return RET_OK;
}

bool_t iot_asrc_get_timer_start_all(void)
{
    return asrc_timer_start_all;
}

void iot_asrc_timer_matrix_start(uint8_t asrc_matrix)
{
    audio_timer_matrix_enable(asrc_matrix);
}

void iot_asrc_bt_select(IOT_ASRC_CHANNEL_ID id)
{
    audio_asrc_bt_select((AUDIO_RX_ASRC_ID)id);
}
