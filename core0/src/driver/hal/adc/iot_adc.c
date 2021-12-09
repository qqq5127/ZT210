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
#include "os_lock.h"
/* hw include */
#include "adi_slave.h"
#include "dtop_ana.h"
#include "meter_adc.h"
#include "pmm_ana.h"
#include "bingo_ana.h"
#include "meter_adc_ana.h"
#include "aud_intf_pwr.h"
#include "aud_glb.h"
#include "bingo_pmm.h"
#include "caldata.h"
#include "gpio.h"

/* hal includes */
#include "iot_dma.h"
#include "iot_adc.h"
#include "iot_timer.h"
#include "driver_dbglog.h"

#ifdef LOW_POWER_ENABLE
#include "generic_list.h"
#include "dev_pm.h"
#endif

#define IOT_ADC_DEBUG              0
#define IOT_ADC_DEFAULT_PORT       0
#define IOT_ADC_DMA_READ_THRESHOLD 4
#define IOT_ADC_DMA_ORIGINAL_2BIT  2
#define IOT_ADC_ANA_GPIO_NUM       4

typedef enum {
    MADC_EXT_INPUT_OFF,
    MADC_EXT_INPUT_0,
    MADC_EXT_INPUT_1,
    MADC_EXT_INPUT_2,
    MADC_EXT_INPUT_3,
} MADC_EXT_INPUT;

enum {
    MADC_INTR_ATB_OFF,
    MADC_INTR_ATB_AON,
    MADC_INTR_ATB_VBAT_4P25,
    MADC_INTR_ATB_VBUS,
};

enum {
    MADC_INTR_CHN_P_40N_OFF = 0,
    MADC_INTR_CHN_P_40N_ATB_ANA = 1,
    MADC_INTR_CHN_P_40N_VCM_P = 6,
};

enum {
    MADC_INTR_CHN_N_40N_OFF,
    MADC_INTR_CHN_N_40N_VCM_N = 6,
    MADC_INTR_CHN_N_40N_VCM_FILTER = 7,
};

enum {
    MADC_ATB_AON_TRI_STATE,
    MADC_ATB_AON_VTEMP = 2,
};

typedef enum {
    IOT_ADC_CLK_ANA_4M,
    IOT_ADC_CLK_ANA_2M,
    IOT_ADC_CLK_ANA_1M,
    IOT_ADC_CLK_ANA_500K,
} IOT_ADC_CLK_ANA;

typedef struct {
    uint8_t selp;
    uint8_t seln;
    uint8_t sel_atb;
    uint8_t seln_pad_mux;
    uint8_t selp_pad_mux;
} iot_adc_ana_chn_cfg_t;

typedef struct {
    IOT_ADC_CLK_ANA in_freq;
    uint8_t sinc_num;
    uint8_t sinc_stg;
    uint8_t sinc_lsh;
    uint8_t int_factor;
} iot_adc_sample_rate_params_t;

static const iot_adc_sample_rate_params_t iot_adc_sample_rate_params[] = {
    {IOT_ADC_CLK_ANA_500K, 249, 3, 6, 1}, {IOT_ADC_CLK_ANA_500K, 124, 3, 10, 1},
    {IOT_ADC_CLK_ANA_2M, 249, 3, 6, 1},   {IOT_ADC_CLK_ANA_2M, 124, 3, 10, 1},
    {IOT_ADC_CLK_ANA_1M, 124, 3, 10, 4},  {IOT_ADC_CLK_ANA_2M, 124, 3, 10, 4},
    {IOT_ADC_CLK_ANA_4M, 124, 3, 10, 4},  {IOT_ADC_CLK_ANA_2M, 7, 3, 26, 1},
    {IOT_ADC_CLK_ANA_4M, 7, 3, 26, 1},
};

static IOT_ADC_WORK_MODE iot_adc_ch_status = IOT_ADC_WORK_MODE_SINGLE;

static os_mutex_h adc_mutex = NULL;

static const madc_sdm_ana_cfg_t madc_sdm_cfg = {

    .bias_int1a_ctrl = 2,
    .bias_int1b_ctrl = 2,
    .bias_int2_ctrl = 2,
    .vrefbuf_ctrl = 1,
    .meter_idac = 0,
    .vcm_ctrl = 3,
    .vref_ctrl = 4,
};

static bool_t iot_adc_inited = false;
static uint32_t iot_adc_dma_ch = 0;
static uint32_t iot_adc_gpio_open = 0;

#ifdef LOW_POWER_ENABLE
static uint32_t iot_adc_restore(uint32_t data)
{
    UNUSED(data);

    /* restore ana gpio to adc mode */
    for (uint8_t i = 0; i < IOT_ADC_ANA_GPIO_NUM; i++) {
        if ((iot_adc_gpio_open & BIT(i)) != 0) {
            gpio_config(i + PIN_ADC_0, GPIO_DIRECTION_INPUT);
            dtop_ana_claim_as_adc(i);
        }
    }

    madc_ana_restore();

    return RET_OK;
}

static struct pm_operation iot_adc_pm = {
    .node = LIST_HEAD_INIT(iot_adc_pm.node),
    .save = NULL,
    .restore = iot_adc_restore,
    .data = (uint32_t)&iot_adc_inited,
};
#endif

static void iot_adc_atb_ana_shutoff(uint8_t phase)
{
    adi_slave_clr_all_bt_afe_rsv();
    bingo_pmm_clr_lp_bias_trim(MADC_CTAL_CUR_BIAS);
    bingo_ana_clr_all_sel_atb_afe();
    bingo_ana_clr_all_d_mic0_atb_ctrl();
    bingo_ana_clr_all_d_mic1_atb_ctrl();
    bingo_ana_clr_all_d_mic2_atb_ctrl();
    bingo_ana_clr_all_d_rdac_l_atb_ctrl();
    bingo_ana_clr_all_d_rdac_r_atb_ctrl();
    madc_ana_clr_d_pho_tia_resv((MADC_ANA_CHANNEL_ID)phase, MADC_D_PHO_TIA_RSV);
    madc_ana_clr_all_d_meter_resv_40n((MADC_ANA_CHANNEL_ID)phase);
    pmm_ana_atb_aon_sel(MADC_ATB_AON_TRI_STATE);
}

static void iot_adc_ana_channel_config(uint8_t phase_no, const iot_adc_ana_chn_cfg_t *cfg)
{
    madc_ana_channel_config((MADC_ANA_CHANNEL_ID)phase_no, cfg->selp_pad_mux, cfg->seln_pad_mux);
    madc_channel_config((MADC_CHANNEL_ID)phase_no, cfg->sel_atb, cfg->selp, cfg->seln);
}

static void iot_adc_ana_extern_input(uint8_t phase_no, iot_adc_ana_chn_cfg_t *cfg, MADC_EXT_INPUT p,
                                     MADC_EXT_INPUT n)
{
    cfg->sel_atb = MADC_INTR_ATB_OFF;

    /* For extern signal p always used */
    assert(p != MADC_EXT_INPUT_OFF);

    cfg->selp_pad_mux = MADC_INTR_CHN_P_40N_OFF;
    cfg->selp = p;

    if (n == MADC_EXT_INPUT_OFF) {
        cfg->seln_pad_mux = MADC_INTR_CHN_N_40N_VCM_FILTER;
    } else {
        cfg->seln_pad_mux = MADC_INTR_CHN_N_40N_OFF;
    }

    cfg->seln = n;

    iot_adc_ana_channel_config(phase_no, cfg);
}

static void iot_adc_ana_vtemp_senser(uint8_t phase_no, iot_adc_ana_chn_cfg_t *cfg)
{
    cfg->sel_atb = MADC_INTR_ATB_AON;
    cfg->selp_pad_mux = MADC_INTR_CHN_P_40N_ATB_ANA;
    cfg->seln_pad_mux = MADC_INTR_CHN_N_40N_VCM_FILTER;
    cfg->selp = MADC_EXT_INPUT_OFF;
    cfg->seln = MADC_EXT_INPUT_OFF;
    iot_adc_ana_channel_config(phase_no, cfg);
    pmm_ana_atb_aon_sel(MADC_ATB_AON_VTEMP);
    pmm_ana_vtemp_sel(false);
}

static void iot_adc_ana_charge_in_voltage(uint8_t phase_no, iot_adc_ana_chn_cfg_t *cfg)
{
    cfg->sel_atb = MADC_INTR_ATB_VBUS;
    cfg->selp_pad_mux = MADC_INTR_CHN_P_40N_OFF;
    cfg->seln_pad_mux = MADC_INTR_CHN_N_40N_VCM_FILTER;
    cfg->selp = MADC_EXT_INPUT_OFF;
    cfg->seln = MADC_EXT_INPUT_OFF;
    iot_adc_ana_channel_config(phase_no, cfg);
}

static void iot_adc_ana_charge_out_voltage(uint8_t phase_no, iot_adc_ana_chn_cfg_t *cfg)
{
    cfg->sel_atb = MADC_INTR_ATB_VBAT_4P25;
    cfg->selp_pad_mux = MADC_INTR_CHN_P_40N_OFF;
    cfg->seln_pad_mux = MADC_INTR_CHN_N_40N_VCM_FILTER;
    cfg->selp = MADC_EXT_INPUT_OFF;
    cfg->seln = MADC_EXT_INPUT_OFF;
    iot_adc_ana_channel_config(phase_no, cfg);
}

static void iot_adc_ana_vad_sndr_mesure(uint8_t phase_no, iot_adc_ana_chn_cfg_t *cfg)
{
    cfg->sel_atb = MADC_INTR_ATB_OFF;
    cfg->selp_pad_mux = MADC_INTR_CHN_P_40N_VCM_P;
    cfg->seln_pad_mux = MADC_INTR_CHN_N_40N_VCM_N;
    cfg->selp = MADC_EXT_INPUT_2;
    cfg->seln = MADC_EXT_INPUT_3;
    iot_adc_ana_channel_config(phase_no, cfg);
}

void iot_adc_open_external_port(uint8_t adc_gpio)
{
    assert(adc_gpio >= PIN_ADC_0 && adc_gpio <= PIN_ADC_3);

    uint8_t gpio = adc_gpio - PIN_ADC_0;
    if ((iot_adc_gpio_open & BIT(gpio)) == 0) {
        gpio_config(adc_gpio, GPIO_DIRECTION_INPUT);
        dtop_ana_claim_as_adc(gpio);
        iot_adc_gpio_open |= BIT(gpio);
    }
}

void iot_adc_ana_chn_sel(uint8_t port, IOT_ADC_SIG_SRC sig_type)
{
    iot_adc_ana_chn_cfg_t cfg;

    switch (sig_type) {
        case IOT_ADC_ALL_OFF:
            iot_adc_atb_ana_shutoff(port);
            break;
        case IOT_ADC_VAD_SNDR_MEASURE:
            iot_adc_ana_vad_sndr_mesure(port, &cfg);
            break;
        case IOT_ADC_CHARGER_IN_VOLTAGE:
            iot_adc_ana_charge_in_voltage(port, &cfg);
            break;
        case IOT_ADC_CHARGER_OUT_VOLTAGE:
            iot_adc_ana_charge_out_voltage(port, &cfg);
            break;
        case IOT_ADC_EXT_SIG_CH0:
            iot_adc_open_external_port(PIN_ADC_0);
            iot_adc_ana_extern_input(port, &cfg, MADC_EXT_INPUT_0, MADC_EXT_INPUT_OFF);
            break;
        case IOT_ADC_EXT_SIG_CH1:
            iot_adc_open_external_port(PIN_ADC_1);
            iot_adc_ana_extern_input(port, &cfg, MADC_EXT_INPUT_1, MADC_EXT_INPUT_OFF);
            break;
        case IOT_ADC_EXT_SIG_CH2:
            iot_adc_open_external_port(PIN_ADC_2);
            iot_adc_ana_extern_input(port, &cfg, MADC_EXT_INPUT_2, MADC_EXT_INPUT_OFF);
            break;
        case IOT_ADC_EXT_SIG_CH3:
            iot_adc_open_external_port(PIN_ADC_3);
            iot_adc_ana_extern_input(port, &cfg, MADC_EXT_INPUT_3, MADC_EXT_INPUT_OFF);
            break;
        case IOT_ADC_EXT_DIFF_CH01:
            iot_adc_open_external_port(PIN_ADC_0);
            iot_adc_open_external_port(PIN_ADC_1);
            iot_adc_ana_extern_input(port, &cfg, MADC_EXT_INPUT_0, MADC_EXT_INPUT_1);
            break;
        case IOT_ADC_EXT_DIFF_CH23:
            iot_adc_open_external_port(PIN_ADC_2);
            iot_adc_open_external_port(PIN_ADC_3);
            iot_adc_ana_extern_input(port, &cfg, MADC_EXT_INPUT_2, MADC_EXT_INPUT_3);
            break;
        case IOT_ADC_VTEMP_SENSER:
            iot_adc_ana_vtemp_senser(port, &cfg);
            break;
        default:
            break;
    }
}

void iot_adc_init(void)
{
    if (iot_adc_inited) {
        return;
    }

    adc_mutex = os_create_mutex(ADC_MID);
    assert(adc_mutex);
    iot_adc_ch_status = IOT_ADC_WORK_MODE_SINGLE;
    iot_adc_inited = true;
#ifdef LOW_POWER_ENABLE
    iot_dev_pm_register(&iot_adc_pm);
#endif
}

void iot_adc_ana_meter_ldo_trim(uint8_t trim_code)
{
    bingo_ana_ldo1p2_vref_trim(trim_code);
}

void iot_adc_ana_sdm_vref_trim(uint8_t port, uint8_t vref_ctrl, uint8_t vcm_ctrl)
{
    madc_sdm_ana_cfg_t cfg;
    cfg.vref_ctrl = vref_ctrl;
    cfg.vcm_ctrl = vcm_ctrl;
    madc_ana_sdm_config((MADC_ANA_CHANNEL_ID)port, &cfg);
}

void iot_adc_ana_lp_cfg(uint8_t port, bool_t enable)
{
    madc_ana_lowpower_enable((MADC_ANA_CHANNEL_ID)port, enable);
}

void iot_adc_dma_channel_link(IOT_ADC_PORT port)
{
    madc_dma_channel_link((MADC_CHANNEL_ID)port);
}

void iot_adc_start(void)
{
    madc_start();
}

void iot_adc_stop(void)
{
    madc_stop();
}

uint8_t iot_adc_ada_dump_module(IOT_ADC_DUMP_DATA_MODULE sel)
{
    madc_dump_select(sel);
    madc_dump_trigger(true);
    return RET_OK;
}

uint32_t iot_adc_dfe_poll(void)
{
    uint32_t data;
    uint32_t n = 0;
    madc_dfe_flag_clear();

    while (!madc_dfe_data_flag()) {
        assert(n <= 100000);
        n++;
    }
    data = madc_dfe_data_value();
    madc_dfe_flag_clear();
    return data;
}

void iot_adc_deinit(void)
{
    bingo_ana_madc_enable(false);
    pmm_ana_vbat_detect_en(false);
    bingo_pmm_adc_ldo_enable(LDO_1P2_MODULE_MADC, false);
    madc_clk_en(false);
    aud_intf_pwr_off((uint8_t)AUDIO_MODULE_METER_ADC);

    if (adc_mutex) {
        os_delete_mutex(adc_mutex);
    }
    iot_adc_inited = false;
    iot_adc_ch_status = IOT_ADC_WORK_MODE_SINGLE;
}

uint8_t iot_adc_dma_config(void)
{
    IOT_DMA_CHANNEL_ID dma_ch;
    uint8_t ret;

    ret = iot_dma_claim_channel(&dma_ch);
    if (ret != RET_OK) {
        return ret;
    }

    ret =
        iot_dma_ch_peri_config(dma_ch, IOT_DMA_TRANS_PERI_TO_MEM, IOT_DMA_DATA_WIDTH_WORD,
                               IOT_DMA_CH_PRIORITY_HIGH, IOT_DMA_PERI_MADC, IOT_DMA_PERI_REQ_NONE);
    if (ret != RET_OK) {
        iot_dma_free_channel(dma_ch);
        return ret;
    }
    madc_dma_config(IOT_ADC_DMA_READ_THRESHOLD, IOT_ADC_DMA_ORIGINAL_2BIT);
    iot_adc_dma_ch = dma_ch;

    return RET_OK;
}

uint32_t iot_adc_single_channel_receive_poll(IOT_ADC_PORT port, IOT_ADC_DATA_MODE mode)
{
    uint32_t data = 0;
    uint32_t n = 0;
    while (!madc_average_flag((MADC_CHANNEL_ID)port)) {
        assert(n <= 100000);
        n++;
    }

    if (mode == IOT_ADC_AVERAGE_DATA) {
        data = madc_data_average((MADC_CHANNEL_ID)port);
    } else if (mode == IOT_ADC_SUMMATION_DATA) {
        data = madc_data_summation((MADC_CHANNEL_ID)port);
    }

    madc_average_clear((MADC_CHANNEL_ID)port);
    return data;
}

uint8_t iot_adc_receive_dma(uint32_t *buffer, uint32_t length, dma_peri_mem_done_callback cb)
{
    if (iot_adc_dma_ch == 0) {
        return RET_INVAL;
    }
    iot_dma_peri_to_mem((IOT_DMA_CHANNEL_ID)iot_adc_dma_ch, buffer,
                        (void *)(madc_get_rx_fifo_dma_addr()), length, cb);
    return RET_OK;
}

static void iot_adc_set_sample_rate(IOT_ADC_SAMPLE_RATE sample_rate)
{
    bingo_ana_madc_frequence_set(iot_adc_sample_rate_params[(uint32_t)sample_rate].in_freq);
    madc_sinc_set(iot_adc_sample_rate_params[(uint32_t)sample_rate].sinc_num,
                  iot_adc_sample_rate_params[(uint32_t)sample_rate].sinc_stg,
                  iot_adc_sample_rate_params[(uint32_t)sample_rate].sinc_lsh,
                  iot_adc_sample_rate_params[(uint32_t)sample_rate].int_factor);
}

void iot_adc_out_vad_link(void)
{
    madc_out_vad((MADC_CHANNEL_ID)IOT_ADC_DEFAULT_PORT);
}

void iot_adc_open(uint8_t ch, IOT_ADC_SAMPLE_RATE sample_rate, uint8_t gain, IOT_ADC_WORK_MODE mode)
{
    uint8_t phase = IOT_ADC_DEFAULT_PORT;

    aud_intf_pwr_on((uint8_t)AUDIO_MODULE_METER_ADC);
    madc_clk_en(true);
    bingo_ana_madc_enable(true);
    iot_adc_set_sample_rate(sample_rate);
    bingo_pmm_adc_ldo_enable(LDO_1P2_MODULE_MADC, true);

    bingo_ana_madc_rc_filter_big_resistor_bypass(true);
    //bypass hpf
    madc_hpf_bypass(true);
    madc_dfe_enable(false);
    if (mode == IOT_ADC_WORK_MODE_MULTI_CONTINUOUS) {
        madc_channel_num(2);
    } else {
        madc_channel_num(1);
    }
    madc_ana_sdm_config((MADC_ANA_CHANNEL_ID)phase, &madc_sdm_cfg);
    madc_ana_d_meter_dit_en((MADC_ANA_CHANNEL_ID)phase, 0);
    iot_adc_ana_chn_sel(phase, (IOT_ADC_SIG_SRC)ch);
    madc_ana_gain_set((MADC_ANA_CHANNEL_ID)phase, gain, true);

    madc_start();
}

void iot_adc_dynamic_cfg_port(uint8_t port, uint8_t ch, uint8_t gain)
{
    madc_ana_sdm_config((MADC_ANA_CHANNEL_ID)port, &madc_sdm_cfg);
    madc_ana_d_meter_dit_en((MADC_ANA_CHANNEL_ID)port, 0);
    iot_adc_ana_chn_sel(port, (IOT_ADC_SIG_SRC)ch);
    madc_ana_gain_set((MADC_ANA_CHANNEL_ID)port, gain, true);
    //read 6 data and discard 5
    madc_set_data_and_discard(port, 5, 5);
}

void iot_adc_close(void)
{
    madc_stop();
    madc_clk_en(false);
    bingo_ana_madc_enable(false);
    bingo_pmm_adc_ldo_enable(LDO_1P2_MODULE_MADC, false);
    aud_intf_pwr_off((uint8_t)AUDIO_MODULE_METER_ADC);
    iot_adc_ch_status = IOT_ADC_WORK_MODE_SINGLE;
}

int32_t iot_adc_poll_data(uint8_t ch, uint8_t gain, uint8_t sum_average)
{
    uint32_t uint_code;
    int32_t int_code = 0;
    uint64_t sum_tmp = 0;
    int i;
    os_acquire_mutex(adc_mutex);
    //fixme:when use vad phase here need be changed
    UNUSED(iot_adc_ch_status);
    uint8_t phase = IOT_ADC_DEFAULT_PORT;

    //fixme:need move to restore
    //audio intf power on vote
    bingo_pmm_adc_ldo_enable(LDO_1P2_MODULE_MADC, true);
    if(ch == IOT_ADC_CHARGER_OUT_VOLTAGE) {
        pmm_ana_vbat_detect_en(true);
    } else if (ch == IOT_ADC_CHARGER_IN_VOLTAGE) {
        pmm_ana_vchg_detect_en(true);
    }
    aud_intf_pwr_on((uint8_t)AUDIO_MODULE_METER_ADC);
    madc_clk_en(true);
    bingo_ana_madc_enable(true);
    //config sample rate,2000000 in
    bingo_ana_madc_frequence_set((uint8_t)IOT_ADC_CLK_ANA_2M);
    //64k out
    madc_sinc_set(124, 3, 10, 4);
    bingo_ana_madc_rc_filter_big_resistor_bypass(true);
    //bypass hpf
    madc_hpf_bypass(true);
    madc_dfe_enable(true);
    madc_channel_num(1);
    madc_ana_sdm_config((MADC_ANA_CHANNEL_ID)phase, &madc_sdm_cfg);
    //dither en can not change to 1
    madc_ana_d_meter_dit_en((MADC_ANA_CHANNEL_ID)phase, 0);
    //move to restore end

    //set ch for phase 0
    iot_adc_ana_chn_sel(phase, (IOT_ADC_SIG_SRC)ch);
    //set gain
    madc_ana_gain_set((MADC_ANA_CHANNEL_ID)phase, gain, true);
    //fixme:vad used need use multi phases and hardware discard
    //madc_set_data_and_discard(phase, 4 + sum_average, 5);
    madc_start();

    //discard some previous data
    for (i = 0; i < 7; i++) {
        iot_adc_dfe_poll();
    }

    for(i = 0; i < sum_average; i++) {
      sum_tmp += iot_adc_dfe_poll();
    }

    uint_code = (uint32_t)(sum_tmp/sum_average) ;

    madc_stop();
    //fixme:need move to save
    bingo_ana_madc_enable(false);
    bingo_pmm_adc_ldo_enable(LDO_1P2_MODULE_MADC, false);
    madc_ana_gain_set((MADC_ANA_CHANNEL_ID)phase, gain, false);
    adi_slave_meter_chn_start(1);
    madc_dfe_enable(false);
    aud_intf_pwr_off((uint8_t)AUDIO_MODULE_METER_ADC);
    if(ch == IOT_ADC_CHARGER_OUT_VOLTAGE) {
        pmm_ana_vbat_detect_en(false);
    } else if (ch == IOT_ADC_CHARGER_IN_VOLTAGE) {
        pmm_ana_vchg_detect_en(false);
    }
    //need move to save end
    os_release_mutex(adc_mutex);

    //sign bit
    if (uint_code & (1 << 23)) {
        int_code = (int32_t)(uint_code | (0xffU << 24));
    } else {
        int_code = (int32_t)(uint_code & 0x7fffff);
    }
#if IOT_ADC_DEBUG
    DBGLOG_DRIVER_INFO("adc-code:%d\n", uint_code);
#endif
    return int_code;
}

float iot_adc_2_mv(uint8_t isdiff, int32_t adc_code)
{
    float sig_mv;
    cal_data_madc adc_cal_data = {0};
    cal_data_madc_get(&adc_cal_data);

    if (isdiff == 1) {
        sig_mv = (adc_code - adc_cal_data.dc_offset_code)
            * (adc_cal_data.vrefpn_minus700mv + 700.0f) / 8388608;
    } else {
        sig_mv = (adc_code - adc_cal_data.dc_offset_code)
                * (adc_cal_data.vrefpn_minus700mv + 700.0f) / 8388608
            + adc_cal_data.vcm_mv;
    }

    return sig_mv;
}
