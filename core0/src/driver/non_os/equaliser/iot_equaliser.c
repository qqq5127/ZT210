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
#include "critical_sec.h"
#include "chip_irq_vector.h"

/* hw includes */
#include "equaliser.h"
#include "math.h"

#include "iot_irq.h"
#include "iot_timer.h"
#include "iot_equaliser.h"

#include "driver_dbglog.h"

#define EQ_START_BAND_MAX  55
#define EQ_BIQUE_NUM_MAX   63
#define IOT_EQ_BAND_MAX    20
#define EQ_DESIGN_POW_2_27 0x8000000
#define EQ_PI              3.14159265359
#define EQ_BURST_LENGTH_POWER_MAX 6
#define EQ_BAND_LENGTH_BYTE (18 * 4)
#define EQ_BAND_EACH_TIME_PROCESS_MAX 1
#define EQ_BIQUED_EACH_TIME_PROCESS_MAX 2
#define EQ_PROCESS_DATA_LEN_MIN 16
#define EQ_PROCESS_DATA_LEN_MULTIPLE 8
#define EQ_WAIT_DONE_TIMEOUT         1000
#define EQ_PROCESS_WAIT_DONE_TIMEOUT 2000

/*lint -esym(754, iot_equaliser_band::state) */
typedef struct iot_equaliser_band {
    int32_t iir0_a[2];
    int32_t iir0_b[3];
    int32_t iir1_a[2];
    int32_t iir1_b[3];
    int32_t state[8];
} iot_equaliser_band_t;

typedef struct iot_equaliser_state {
    bool_t in_use;
    uint8_t link_band_start;
    uint8_t start_band;
    uint8_t bique_num;
    IOT_EQ_FLT_MODE flt_mode;
    IOT_EQ_DATA_MODE data_mode;
    iot_equaliser_done_callback cb;
} iot_equaliser_state_t;

typedef struct iot_equaliser_cfg {
    uint8_t start_band;
    uint8_t bique_num;
    IOT_EQ_FLT_MODE flt_mode;
    IOT_EQ_DATA_MODE data_mode;
    IOT_EQ_TRANS_MODE trans_mode;
    uint8_t *src;
    uint8_t *dst;
    uint32_t data_length;
} iot_equaliser_cfg_t;

typedef struct iot_equaliser_coeff_design_param {
    uint32_t *freq;
    float *gain;
    float *q;
    uint8_t *type;
    float overall_gain;
    int32_t sample_rate;
} iot_equaliser_coeff_design_param_t;

static iot_irq_t eq_int_isr;
static iot_equaliser_band_t eq_coeff_band[IOT_EQ_BAND_MAX] = {0};
static iot_equaliser_state_t equaliser;
static void iot_equaliser_interrupt_config(void);
static void iot_equaliser_data_done_handler(void);
static void iot_equaliser_coeff_done_handler(void);
static uint32_t iot_equaliser_isr_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_equaliser_isr_handler);
static uint8_t iot_equaliser_coeff_design(IOT_EQ_MODE mode, iot_equaliser_band_t *band_buf,
                                          const iot_equaliser_coeff_design_param_t *param);
static void iot_equaliser_design(IOT_EQ_TYPE type, uint32_t freq, float q, float gain, int32_t fs, int32_t *b,
                                 int32_t *a);
static void iot_equaliser_design_overall_gain(IOT_EQ_TYPE type, uint32_t freq, float q, float gain, float overall_gain, int32_t fs, int32_t *b,
                                 int32_t *a);
static uint8_t iot_equaliser_cal(const iot_equaliser_cfg_t *cfg);
static int8_t iot_equaliser_config(const iot_equaliser_cfg_t *cfg);

void iot_equaliser_init(void)
{
    equaliser.in_use = false;
    equaliser.start_band = 0;
    equaliser.bique_num = 0;
    equaliser.link_band_start = 0;
    equaliser.flt_mode = IOT_EQ_FLOATING_POINT;
    equaliser.data_mode = IOT_EQ_32_BIT;
    memset(eq_coeff_band, 0x00, sizeof(eq_coeff_band));
}

void iot_equaliser_deinit(void)
{
    iot_irq_mask(eq_int_isr);
    iot_irq_delete(eq_int_isr);
}

uint8_t iot_equaliser_speaker_open(iot_equaliser_speaker_param_t *param)
{
    uint8_t band_number;
    iot_equaliser_band_t *p_band = &eq_coeff_band[0];
    iot_equaliser_coeff_design_param_t design = {0};

    design.freq = param->freq;
    design.gain = param->gain;
    design.q = param->q;
    design.sample_rate = param->sample_rate;
    design.overall_gain = param->overall_gain;
    design.type = param->type;
    band_number = iot_equaliser_coeff_design(IOT_EQ_SPEAKER, p_band, &design);
    equaliser.link_band_start = band_number;

    return RET_OK;
}

int8_t iot_equaliser_music_open(iot_equaliser_music_param_t *param)
{
    int8_t ret;
    uint8_t band_number;
    iot_equaliser_cfg_t cfg = {0};
    iot_equaliser_coeff_design_param_t design = {0};
    iot_equaliser_band_t *p_band = &eq_coeff_band[equaliser.link_band_start];

    design.freq = param->freq;
    design.gain = param->gain;
    design.q = param->q;
    design.overall_gain = 0;
    design.sample_rate = param->sample_rate;
    design.type = param->type;
    band_number = iot_equaliser_coeff_design(IOT_EQ_MUSIC, p_band, &design);
    band_number = equaliser.link_band_start + band_number;
    DBGLOG_DRIVER_INFO("[EQ] iot_equaliser_music_open %d %d\n", equaliser.link_band_start, band_number);

    cfg.start_band = 0;
    cfg.bique_num = (uint8_t)(band_number * 2);
    cfg.flt_mode = param->flt_mode;
    cfg.data_mode = param->data_mode;
    cfg.trans_mode = param->trans_mode;
    cfg.src = (uint8_t *)eq_coeff_band;
    cfg.dst = (uint8_t *)eq_coeff_band;
    cfg.data_length = ((uint32_t)band_number) * sizeof(iot_equaliser_band_t);

    if(param->trans_mode != IOT_EQ_TRANS_INTERRUPT) {
        equaliser.cb = NULL;
        param->cb = NULL;
    } else {
        iot_equaliser_interrupt_config();
        equaliser.cb = param->cb;
    }

    ret = iot_equaliser_config(&cfg);
    if (!ret) {
        ret = (int8_t)band_number;
    }

    return ret;
}

void iot_equaliser_close(void)
{
    eq_set_coef_cmd(0, 0, 0, 0);
    eq_disable();
    equaliser.in_use = false;
}

uint8_t iot_equaliser_process(const iot_equaliser_process_t *process)
{
    uint8_t ret = RET_OK;
    uint8_t eq_num_cnt = 0;
    uint16_t eq_timeout_cnt = 0;
    uint8_t *p_eq_coeff_band;
    iot_equaliser_cfg_t cfg;
    if(process->data_length < EQ_PROCESS_DATA_LEN_MIN || process->data_length % EQ_PROCESS_DATA_LEN_MULTIPLE != 0) {
        DBGLOG_DRIVER_INFO("[EQ] iot_equaliser_process invaled data length:%d\n", process->data_length);
        return RET_INVAL;
    }

    p_eq_coeff_band = (uint8_t *)eq_coeff_band;
    cfg.flt_mode = equaliser.flt_mode;
    cfg.data_mode = equaliser.data_mode;
    cfg.src = (uint8_t *)process->data_in;
    cfg.dst = (uint8_t *)process->data_out;
    cfg.data_length = process->data_length;
    cfg.trans_mode = process->trans_mode;

    eq_set_srst(true);
    eq_set_srst(false);

    eq_set_coef_rd_addr(p_eq_coeff_band);
    eq_set_coef_wr_addr(p_eq_coeff_band);
    eq_set_coef_cmd(0, 1, 0, 1);

    while(!eq_get_it_coeff_done_raw()) {
        iot_timer_delay_us(10);
        if(eq_timeout_cnt++ > EQ_PROCESS_WAIT_DONE_TIMEOUT) {
            assert(0);
        }
    }

    if (eq_timeout_cnt > EQ_WAIT_DONE_TIMEOUT) {
        DBGLOG_DRIVER_ERROR("[EQ] wait eq process done wait:%dus\n", eq_timeout_cnt * 10);
    }

    eq_set_it_coef_done_clr(1);
    eq_set_coef_cmd(0, 0, 0, 0);

    if (process->data_length == EQ_PROCESS_DATA_LEN_MIN) {
        uint8_t eq_num = equaliser.bique_num;
        cfg.start_band = 0;
        while(eq_num - eq_num_cnt) {
            if((eq_num - eq_num_cnt) < EQ_BIQUED_EACH_TIME_PROCESS_MAX) {
                cfg.bique_num = eq_num - eq_num_cnt;
            } else if((eq_num - eq_num_cnt) >= EQ_BIQUED_EACH_TIME_PROCESS_MAX){
                cfg.bique_num = EQ_BIQUED_EACH_TIME_PROCESS_MAX;
            }
            if(iot_equaliser_cal(&cfg) != RET_OK) {
                DBGLOG_DRIVER_INFO("[EQ] iot_equaliser_process data length 16 failed\n");
                return RET_INVAL;
            }
            eq_set_srst(true);
            eq_set_srst(false);
            cfg.start_band += EQ_BAND_EACH_TIME_PROCESS_MAX;
            p_eq_coeff_band += EQ_BAND_LENGTH_BYTE;
            eq_set_coef_wr_addr(p_eq_coeff_band);
            eq_num_cnt += EQ_BIQUED_EACH_TIME_PROCESS_MAX;
        }
        cfg.start_band = 0;
        cfg.bique_num = equaliser.bique_num;
        eq_set_mode(cfg.start_band, cfg.bique_num, cfg.data_mode, cfg.flt_mode, (uint16_t)cfg.data_length);
        eq_set_coef_wr_addr((uint8_t *)eq_coeff_band);
        ret = RET_OK;
    } else {
        cpu_critical_enter();
        cfg.start_band = equaliser.start_band;
        cfg.bique_num = equaliser.bique_num;

        if(process->trans_mode == IOT_EQ_TRANS_INTERRUPT) {
            equaliser.cb = process->cb;
        }
        cpu_critical_exit();
        ret = iot_equaliser_cal(&cfg);
    }
    iot_equaliser_get_coeff_state((uint8_t *)eq_coeff_band);
    return ret;
}

void iot_equaliser_get_coeff_state(uint8_t *eq_coef_state)
{
    uint16_t eq_timeout_cnt = 0;
    eq_set_coef_wr_addr(eq_coef_state);
    eq_set_coef_cmd(0, 1, 1, 1);
    while(!eq_get_it_coeff_done_raw()) {
        iot_timer_delay_us(10);
        if(eq_timeout_cnt++ > EQ_WAIT_DONE_TIMEOUT) {
            assert(0);
        }
    }
    eq_set_it_coef_done_clr(1);
    eq_set_coef_cmd(0, 0, 0, 0);
}

uint8_t iot_equaliser_set_burst_length(uint8_t powers)
{
    if(powers > EQ_BURST_LENGTH_POWER_MAX) {
        return RET_INVAL;
    }
    eq_set_ahb_bytes(powers);

    return RET_OK;
}

void iot_eq_set_data_mode(IOT_EQ_DATA_MODE mode)
{
    eq_set_data_mode(mode);
}

static uint32_t iot_equaliser_isr_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);
    if (eq_get_it_coef_done()) {
        eq_set_it_coef_done_clr(1);
        iot_equaliser_coeff_done_handler();
    }
    if (eq_get_it_data_done()) {
        eq_set_it_data_done_clr(1);
        iot_equaliser_data_done_handler();
    }

    return 0;
}

static void iot_equaliser_coeff_done_handler(void)
{
    eq_set_coef_cmd(0, 0, 0, 0);
    if (equaliser.cb != NULL) {
        equaliser.cb();
    }
}

static void iot_equaliser_data_done_handler(void)
{
    eq_set_done(false);
    if (equaliser.cb != NULL) {
        equaliser.cb();
    }
}

static void iot_equaliser_interrupt_config(void)
{
    eq_int_isr = iot_irq_create(EQ_DONE_INT, 0, iot_equaliser_isr_handler);
    eq_set_it_coef_done_clr(1);
    eq_set_it_data_done_clr(1);
    iot_irq_unmask(eq_int_isr);
}

static void iot_equaliser_design(IOT_EQ_TYPE type, uint32_t freq, float q, float gain, int32_t fs,
                                 int32_t *b, int32_t *a)
{
    float b0;
    float b1;
    float b2;
    float a0;
    float a1;
    float a2;
    float temp;

    temp = (float)(2 * EQ_PI * freq);
    float w0 = (float)(temp / fs);
    float alpha = (float)(sin_float(w0) * 0.5f) / q;
    float gain_value = pow_10(0.025f * gain);
    if(type == IOT_EQ_PEQ) {
        float alpha_temp0 = alpha * gain_value;
        float alpha_temp1 = alpha / gain_value;

        b0 = 1.0f + alpha_temp0;
        b1 = -2.0f * cos_float(w0);
        b2 = 1.0f - alpha_temp0;
        a0 = 1.0f / (1.0f + alpha_temp1);
        a1 = b1;
        a2 = 1.0f - alpha_temp1;
    } else if(type == IOT_EQ_LOW_SHELF) {
        float pow_data = 2 * pow_10(0.0125f * gain) * alpha;
        float cos_data = cos_float(w0);
        float gain_reduce_cos_data = (gain_value - 1) *cos_data;
        float gain_plus_data = (gain_value + 1);

        b0 = gain_value * ((gain_plus_data - gain_reduce_cos_data) + pow_data);
        b1 = 2 * gain_value * ((gain_value - 1) - gain_plus_data * cos_data);
        b2 = gain_value * ((gain_plus_data - gain_reduce_cos_data) - pow_data);
        a0 = 1.0f / (gain_plus_data + gain_reduce_cos_data + pow_data);
        a1 = -2 * ((gain_value - 1) + gain_plus_data * cos_data);
        a2 = gain_plus_data + gain_reduce_cos_data - pow_data;

    } else {
        float pow_data = 2 * pow_10(0.0125f * gain) * alpha;
        float cos_data = cos_float(w0);
        float gain_reduce_cos_data = (gain_value - 1) *cos_data;
        float gain_plus_data = (gain_value + 1);

        b0 = gain_value * (gain_plus_data + gain_reduce_cos_data + pow_data);
        b1 = -2 * gain_value * ((gain_value - 1) + gain_plus_data * cos_data);
        b2 = gain_value * (gain_plus_data + gain_reduce_cos_data - pow_data);
        a0 = 1.0f / ((gain_plus_data - gain_reduce_cos_data) + pow_data);
        a1 = 2 * ((gain_value - 1) - gain_plus_data * cos_data);
        a2 = (gain_plus_data - gain_reduce_cos_data) - pow_data;
    }

    b[0] = (int32_t)(EQ_DESIGN_POW_2_27 * (b0 * a0));
    b[1] = (int32_t)(EQ_DESIGN_POW_2_27 * (b1 * a0));
    b[2] = (int32_t)(EQ_DESIGN_POW_2_27 * (b2 * a0));
    a[0] = (int32_t)(EQ_DESIGN_POW_2_27 * (a1 * a0));
    a[1] = (int32_t)(EQ_DESIGN_POW_2_27 * (a2 * a0));
}


static void iot_equaliser_design_overall_gain(IOT_EQ_TYPE type, uint32_t freq, float q, float gain, float overall_gain, int32_t fs, int32_t *b,
                                 int32_t *a)
{
    iot_equaliser_design(type, freq, q, gain, fs, b, a);
    float pow_data_overall_gain = pow_10(0.05f * overall_gain);
    b[0] = (int32_t)(pow_data_overall_gain * b[0]);
    b[1] = (int32_t)(pow_data_overall_gain * b[1]);
    b[2] = (int32_t)(pow_data_overall_gain * b[2]);
}

static uint8_t iot_equaliser_coeff_design(IOT_EQ_MODE mode, iot_equaliser_band_t *band_buf,
                                          const iot_equaliser_coeff_design_param_t *param)
{
    int i = 0;
    int32_t fs;
    uint8_t band_index = 0;
    uint8_t eq_band_num;
    uint8_t eq_bique_num = 0;
    iot_equaliser_band_t *eq_band;
    static float prev_overall_gain = 0;

    eq_band = band_buf;
    fs = param->sample_rate;
    if(mode == IOT_EQ_SPEAKER) {
        for (i = 0; i < IOT_EQ_ES_MAX; i++) {
            if (param->freq[i] > 0) {
                band_index = eq_bique_num / 2;
                if(eq_bique_num == 0) {
                    iot_equaliser_design_overall_gain((IOT_EQ_TYPE)param->type[i], param->freq[i], (float)param->q[i], (float)param->gain[i], param->overall_gain, fs,
                                    &eq_band[band_index].iir0_b[0],
                                    &eq_band[band_index].iir0_a[0]);
                }else if (!(eq_bique_num % 2)) {
                    iot_equaliser_design((IOT_EQ_TYPE)param->type[i], param->freq[i], (float)param->q[i], (float)param->gain[i], fs,
                                        &eq_band[band_index].iir0_b[0],
                                        &eq_band[band_index].iir0_a[0]);   //even
                } else {
                    iot_equaliser_design((IOT_EQ_TYPE)param->type[i], param->freq[i], (float)param->q[i], (float)param->gain[i], fs,
                                        &eq_band[band_index].iir1_b[0],
                                        &eq_band[band_index].iir1_a[0]);   //odd
                }
                eq_bique_num++;
            }
        }
        if(eq_bique_num == 0) {
            prev_overall_gain = param->overall_gain;
        }
    } else {
        for (i = 0; i < IOT_EQ_EM_MAX; i++) {
            if (param->freq[i] > 0) {
                band_index = eq_bique_num >> 1;
                if (!(eq_bique_num % 2)) {
                    iot_equaliser_design((IOT_EQ_TYPE)param->type[i], param->freq[i], (float)param->q[i], (float)param->gain[i], fs,
                                        &eq_band[band_index].iir0_b[0],
                                        &eq_band[band_index].iir0_a[0]);   //even
                } else {
                    iot_equaliser_design((IOT_EQ_TYPE)param->type[i], param->freq[i], (float)param->q[i], (float)param->gain[i], fs,
                                        &eq_band[band_index].iir1_b[0],
                                        &eq_band[band_index].iir1_a[0]);   //odd
                }
                eq_bique_num++;
            }
        }
        if(eq_bique_num == 0) {
            eq_band[0].iir0_a[0] = 0x00000000;
            eq_band[0].iir0_a[1] = 0x00000000;
            eq_band[0].iir0_b[0] = (int32_t)(0x08000000 * pow_10(0.05f * prev_overall_gain));
            eq_band[0].iir0_b[1] = 0x00000000;
            eq_band[0].iir0_b[2] = 0x00000000;

            eq_band[0].iir1_a[0] = 0x00000000;
            eq_band[0].iir1_a[1] = 0x00000000;
            eq_band[0].iir1_b[0] = 0x08000000;
            eq_band[0].iir1_b[1] = 0x00000000;
            eq_band[0].iir1_b[2] = 0x00000000;

            eq_bique_num = 2;
        }
    }

    if (eq_bique_num % 2) {
        eq_band[band_index].iir1_a[0] = 0x00000000;
        eq_band[band_index].iir1_a[1] = 0x00000000;
        eq_band[band_index].iir1_b[0] = 0x08000000;
        eq_band[band_index].iir1_b[1] = 0x00000000;
        eq_band[band_index].iir1_b[2] = 0x00000000;
    }
    eq_band_num = ((eq_bique_num + 1) >> 1);

    return eq_band_num;
}

static int8_t iot_equaliser_config(const iot_equaliser_cfg_t *cfg)
{
    if (cfg->start_band > EQ_START_BAND_MAX || cfg->bique_num > EQ_BIQUE_NUM_MAX) {
        return -RET_INVAL;
    }
    uint16_t eq_timeout_cnt = 0;

    if(!equaliser.in_use) {
        eq_enable();
    }
    equaliser.in_use = true;

    eq_set_mode(cfg->start_band, cfg->bique_num, cfg->data_mode, cfg->flt_mode, (uint16_t)cfg->data_length);

    eq_set_coef_rd_addr(cfg->src);
    eq_set_coef_wr_addr(cfg->dst);

    eq_set_shift(0, 0);
    eq_set_eq_flt(0, 0, 0, 0);

    eq_set_it_enable(1, 1);
    eq_set_srst(true);
    eq_set_srst(false);
    eq_set_coef_cmd(0, 1, 0, 1);

    cpu_critical_enter();
    equaliser.start_band = cfg->start_band;
    equaliser.bique_num = cfg->bique_num;
    equaliser.flt_mode = cfg->flt_mode;
    equaliser.data_mode = cfg->data_mode;
    cpu_critical_exit();

    if(cfg->trans_mode != IOT_EQ_TRANS_INTERRUPT) {
        while(!eq_get_it_coeff_done_raw()){
            iot_timer_delay_us(10);
            if(eq_timeout_cnt++ > EQ_WAIT_DONE_TIMEOUT) {
                for(uint16_t i = 0; i < cfg->data_length; i++) {
                    DBGLOG_DRIVER_ERROR("id:%d, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", i,
                    eq_coeff_band[i].iir0_a[0], eq_coeff_band[i].iir0_a[1], eq_coeff_band[i].iir0_b[0],
                    eq_coeff_band[i].iir0_b[1], eq_coeff_band[i].iir0_b[2], eq_coeff_band[i].iir1_a[0],
                    eq_coeff_band[i].iir1_a[1], eq_coeff_band[i].iir1_b[0], eq_coeff_band[i].iir1_b[1],
                    eq_coeff_band[i].iir1_b[2]);
                }
                DBGLOG_DRIVER_ERROR("src addr:0x%x, data_length:%d\n", cfg->src, cfg->data_length);
                assert(0);
            }
        }
        eq_set_it_coef_done_clr(1);
        eq_set_coef_cmd(0, 0, 0, 0);
    }

    return RET_OK;
}

uint8_t iot_equaliser_retry_config_poll(IOT_EQ_DATA_MODE data_mode)
{
    uint16_t coeff_length = ((equaliser.bique_num + 1) >> 1) * sizeof(iot_equaliser_band_t);

    eq_set_it_data_done_clr(1);
    eq_set_done(false);

    eq_set_mode(equaliser.start_band, equaliser.bique_num, data_mode, equaliser.flt_mode, coeff_length);

    eq_set_shift(0, 0);
    eq_set_eq_flt(0, 0, 0, 0);

    eq_set_it_enable(1, 1);
    eq_set_srst(true);
    eq_set_srst(false);

    for(int i = 0; i < 8; i++) {
        DBGLOG_DRIVER_INFO("retry:id:%d, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", i,
        eq_coeff_band[i].iir0_a[0], eq_coeff_band[i].iir0_a[1], eq_coeff_band[i].iir0_b[0],
        eq_coeff_band[i].iir0_b[1], eq_coeff_band[i].iir0_b[2], eq_coeff_band[i].iir1_a[0],
        eq_coeff_band[i].iir1_a[1], eq_coeff_band[i].iir1_b[0], eq_coeff_band[i].iir1_b[1],  eq_coeff_band[i].iir1_b[2]);
    }

    return RET_OK;
}

static uint8_t iot_equaliser_cal(const iot_equaliser_cfg_t *cfg)
{
    if (cfg->start_band > EQ_START_BAND_MAX || cfg->bique_num > EQ_BIQUE_NUM_MAX) {
        DBGLOG_DRIVER_INFO("[EQ] iot_equaliser_cal start_band or cfg->bique_num invaled\n");
        return RET_INVAL;
    }
    uint8_t eq_retry_process = 0;

    eq_set_srst(true);
    eq_set_srst(false);
    eq_set_mode(cfg->start_band, cfg->bique_num, cfg->data_mode, cfg->flt_mode, (uint16_t)cfg->data_length);
    eq_set_data_rd_addr(cfg->src);
    eq_set_data_wr_addr(cfg->dst);
    eq_set_start(true);

    if(cfg->trans_mode != IOT_EQ_TRANS_INTERRUPT) {
        /* wait to eq cal finish */
        int wait_done_counter = 5000;
        while(!eq_get_it_data_done_raw()) {
            wait_done_counter--;
            /* when the eq has not been processed for more than a overflow time,
             other modules may overflow because they wait too long. In order to
             prevent this from being sent, we will redistribute the eq configuration
             coefficient and saved status after polling 2000 times. Let the eq
             finish the processing quickly.  of course, we have to make sure that
             the reprocessing also occurs within the overflow time.under the test,
             two pieces of music were played with only one again reprocessing at 1000 polls */
            if (wait_done_counter == 0) {
                if(eq_retry_process) {
                    DBGLOG_DRIVER_INFO("[EQ] iot_equaliser_cal retry failed\n");
                    return RET_FAIL;
                }
                DBGLOG_DRIVER_INFO("[EQ] process retry,data lenght:%d\n", cfg->data_length);
                iot_equaliser_retry_config_poll(cfg->data_mode);
                eq_set_mode(cfg->start_band, cfg->bique_num, cfg->data_mode, cfg->flt_mode,
                            (uint16_t)cfg->data_length);
                eq_set_data_rd_addr(cfg->src);
                eq_set_data_wr_addr(cfg->dst);
                eq_set_coef_cmd(0, 0, 0, 0);
                eq_set_start(true);
                eq_retry_process = 1;
                wait_done_counter = 8000;
            }
        }
        eq_set_it_data_done_clr(1);
        eq_set_done(false);
    }

    return RET_OK;
}
