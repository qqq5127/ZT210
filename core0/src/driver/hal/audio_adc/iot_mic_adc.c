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
#include "os_utils.h"
#include "os_timer.h"
#include "critical_sec.h"

/* hw includes */
#include "bingo_ana.h"
#include "adi_slave.h"
#include "bingo_pmm.h"
#include "caldata.h"

#include "iot_timer.h"
#include "iot_mic_adc.h"
#include "iot_resource.h"

#include "driver_dbglog.h"

#define MIC_VOLTAGE_COMMON_VALUE      0
#define MIC_VOLTAGE_REFERENCE_VALUE   7
#define IOT_MIC_BITMAP_MAX            0x07
#define IOT_MIC_ADC_SST_TIMER_MS      40
#define IOT_MIC_ADC_VCM_SET_MS        10
#define MIC_RC_MAX_CAP                0x3f
#define MIC_OPEN_CNT_MAX              2

typedef struct iot_mic_adc_state {
    timer_id_t sst_timer_id[MIC_OPEN_CNT_MAX];
    timer_id_t vcm_set_timer_id[MIC_OPEN_CNT_MAX];
    iot_mic_adc_timer_done_callback mic_cb[MIC_OPEN_CNT_MAX];
    uint32_t mic_open_map;
    uint32_t sst_timer_mask;
    uint8_t sst_timer_cnt;
    uint8_t mic_open_cnt;
    uint8_t micbias_used_mask;
    uint8_t mic_used_mask;
    uint8_t really_used_micbias_map;
    uint8_t current_mic_map[MIC_OPEN_CNT_MAX];
    uint8_t first_init:1;
    uint8_t ana_init_flag:1;
    int8_t cur_ana_gain;
} iot_mic_adc_state_t;

static iot_mic_adc_state_t mic_adc_state;

/* delay 10ms */
static void iot_mic_adc_sst_timer_cb(timer_id_t timer_id, void *p_param)
{
    UNUSED(timer_id);
    UNUSED(p_param);
    uint32_t mic_map;
    uint8_t sst_timer_cnt;
    /* mic_open_cnt had started counting before this,
    sst_timer_cnt only started counting here, and the open mic currently
    allows only twice simultaneous mic on*/
    assert((mic_adc_state.mic_open_cnt > 0) && (mic_adc_state.mic_open_cnt < 3));
    assert((mic_adc_state.sst_timer_cnt < 2));

    cpu_critical_enter();
    mic_adc_state.sst_timer_mask |= BIT(mic_adc_state.sst_timer_cnt);
    mic_adc_state.sst_timer_cnt++;
    sst_timer_cnt = mic_adc_state.sst_timer_cnt;
    mic_map = (uint32_t)mic_adc_state.current_mic_map[mic_adc_state.sst_timer_cnt - 1];
    cpu_critical_exit();

    for (IOT_MIC_ADC i = IOT_MIC_ADC_0; i < IOT_MIC_ADC_MAX; i++) {
        BINGO_ANA_MIC_CHANNEL id = (BINGO_ANA_MIC_CHANNEL)i;
        if (mic_map & BIT(id)) {
            bingo_ana_mic_pga_lock_prevent(id, false);
        }
    }
    if(sst_timer_cnt == 1) {
        os_start_timer(mic_adc_state.vcm_set_timer_id[0], IOT_MIC_ADC_VCM_SET_MS);
    } else {
        os_start_timer(mic_adc_state.vcm_set_timer_id[1], IOT_MIC_ADC_VCM_SET_MS);
    }
}

static void iot_mic_adc_vcm_set_timer_cb(timer_id_t timer_id, void *p_param)
{
    UNUSED(timer_id);
    UNUSED(p_param);

    uint32_t mic_map;
    iot_mic_adc_timer_done_callback mic_callback = NULL;

    cpu_critical_enter();
    if(mic_adc_state.mic_open_map == BIT(1)){
        mic_callback = mic_adc_state.mic_cb[1];
        mic_adc_state.mic_cb[1] = NULL;
        mic_adc_state.mic_open_map &= ~BIT(1);
    } else {
        mic_callback = mic_adc_state.mic_cb[0];
        mic_adc_state.mic_cb[0] = NULL;
        mic_adc_state.mic_open_map &= ~0x01;
    }

    if(mic_adc_state.sst_timer_mask == BIT(1)) {
        mic_map = mic_adc_state.current_mic_map[1];
        mic_adc_state.sst_timer_mask &= ~BIT(1);
    } else {
        mic_map = mic_adc_state.current_mic_map[0];
        mic_adc_state.sst_timer_mask &= ~0x01;
    }
    mic_adc_state.sst_timer_cnt--;
    mic_adc_state.mic_open_cnt--;
    cpu_critical_exit();

    for (uint8_t chid = 0; chid < (uint8_t)IOT_MIC_ADC_MAX; chid++) {
        if (mic_map & BIT(chid)) {
            DBGLOG_DRIVER_INFO("[AUDIO ADC] mic id: %d, cur_ana_gain: 0x%x\n", chid, mic_adc_state.cur_ana_gain);
            bingo_ana_mic_pga_gain_ctrl((BINGO_ANA_MIC_CHANNEL)chid,
                                            (BINGO_ANA_MIC_GAIN)mic_adc_state.cur_ana_gain);
            bingo_ana_mic_adc_dem_enable((BINGO_ANA_MIC_CHANNEL)chid, true);
            bingo_ana_mic_adc_vref_ctrl((BINGO_ANA_MIC_CHANNEL)chid, MIC_VOLTAGE_REFERENCE_VALUE);
            bingo_ana_mic_vcm_ctrl((BINGO_ANA_MIC_CHANNEL)chid, MIC_VOLTAGE_COMMON_VALUE);
        }
    }
    if (mic_callback != NULL) {
        mic_callback();
    }
}

uint8_t iot_mic_adc_ana_gain_init(int8_t ana_gain)
{
    uint8_t ret = RET_INVAL;

    if (ana_gain < IOT_MIC_ADC_GAIN_FORBID) {
        if (mic_adc_state.ana_init_flag == 0) {
            mic_adc_state.ana_init_flag = 1;
        }

        mic_adc_state.cur_ana_gain = ana_gain;

        ret = RET_OK;
    }

    return ret;
}

void iot_mic_adc_init(void)
{
    mic_adc_state.mic_used_mask = 0;
    mic_adc_state.micbias_used_mask = 0;
    mic_adc_state.current_mic_map[0] = 0;
    mic_adc_state.current_mic_map[1] = 0;
    mic_adc_state.really_used_micbias_map = 0;
    mic_adc_state.mic_open_cnt = 0;
    mic_adc_state.sst_timer_cnt = 0;
    mic_adc_state.sst_timer_mask = 0;

    if (mic_adc_state.first_init == 0) {
        mic_adc_state.first_init = 1;

        if (mic_adc_state.ana_init_flag == 0) {
            mic_adc_state.cur_ana_gain = IOT_MIC_ADC_GAIN_0DB;
        }
        mic_adc_state.sst_timer_id[0] =
            os_create_timer(IOT_MIC_ADC_SST_MID, false, iot_mic_adc_sst_timer_cb, NULL);
        mic_adc_state.sst_timer_id[1] =
            os_create_timer(IOT_MIC_ADC_SST_MID, false, iot_mic_adc_sst_timer_cb, NULL);
        mic_adc_state.vcm_set_timer_id[0] =
            os_create_timer(IOT_MIC_ADC_VCM_SET_MID, false, iot_mic_adc_vcm_set_timer_cb, NULL);
        mic_adc_state.vcm_set_timer_id[1] =
            os_create_timer(IOT_MIC_ADC_VCM_SET_MID, false, iot_mic_adc_vcm_set_timer_cb, NULL);
    }
}

void iot_mic_adc_pga_init(uint8_t mic_map)
{
    for (IOT_MIC_ADC i = IOT_MIC_ADC_0; i < IOT_MIC_ADC_MAX; i++) {
        BINGO_ANA_MIC_CHANNEL id = (BINGO_ANA_MIC_CHANNEL)i;
        if (mic_map & BIT(id)) {
            bingo_ana_mic_pga_gain_ctrl(id, (BINGO_ANA_MIC_GAIN)mic_adc_state.cur_ana_gain);
            bingo_ana_mic_pga_enable(id, true);
            bingo_ana_mic_adc_enable(id, true);

            bingo_ana_mic_adc_sst_n(id, true);
            bingo_ana_mic_pga_lock_prevent(id, true);
            /* delay 200us */
            iot_timer_delay_us(200);
            bingo_ana_mic_adc_sst_n(id, false);
        }
    }
    /*delay 200+39800 = 40ms*/
    // os_delay(40);
    if(mic_adc_state.mic_open_map == 1) {
        os_start_timer(mic_adc_state.sst_timer_id[0], IOT_MIC_ADC_SST_TIMER_MS);
    } else {
        os_start_timer(mic_adc_state.sst_timer_id[1], IOT_MIC_ADC_SST_TIMER_MS);
    }
}

/**
 * @brief mic_adc rc tuning, runs when power on each time
 */
uint32_t iot_mic_rc_tuning(void)
{
    uint32_t ret;

    /*d_ct_sdm_rc_tune_target<0> start rc tuning*/
    bingo_ana_ct_sdm_rc_tune_target(1);
    bingo_ana_ct_sdm_c_code_sel(true);
    bingo_ana_ct_sdm_rc_tune_enable(true);
    iot_timer_delay_us(40);

    /* read mic adc integrator's
    capacitor calibration results to digital */
    ret = adi_slave_mic_rc_cap() & MIC_RC_MAX_CAP;
    assert(ret != MIC_RC_MAX_CAP);

    bingo_ana_ct_sdm_rc_tune_enable(false);
    return RET_OK;
}

/*@function:mic bias switch
@micbias_out: select micbias output mode
*/
uint8_t iot_mic_bias_switch_on(uint8_t mic_map, uint8_t micbias_map)
{
    uint8_t micbias_mask = 0;

    if (mic_adc_state.mic_used_mask & mic_map) {
        return RET_BUSY;
    } else if (mic_map == 0) {
        return RET_INVAL;
    }
    mic_adc_state.mic_used_mask |= mic_map;
    mic_adc_state.mic_used_mask &= IOT_MIC_BITMAP_MAX;

    for (uint8_t i = 0; i < IOT_MIC_ADC_MAX; i++) {
        if(mic_map & BIT(i)) {
            mic_adc_state.really_used_micbias_map |= (micbias_map & BIT(i));
        }
        if (mic_adc_state.mic_used_mask & BIT(i)) {
            /**
             * if the mic(i) is detected by having the need to turn on the micbias,
             * to obtain the value of the micbias corresponding to the mic. .
             * if value is 0 put 1 on micbias_mask bit0,
             * if value is 1 put 1 on micbias_mask bit1.
             */
            micbias_mask |= (uint8_t)BIT((BIT(i) & mic_adc_state.really_used_micbias_map) ? 1 : 0);
        }
    }

    /* we need to find the micbias value corresponding to mic and close
    it when needed to close without affecting the use of other mic.
    micbias0 is already bound to the ldo and cannot operate via a register,
    micbias1 is not used, so the bit of the corresponding micbias1 in the register is meaningless,
    we are actually using micbias0 and micbias2,micbias2 if using the corresponding bit in the register in 1
    */
    switch (micbias_mask) {
        case BINGO_ANA_MICBIAS_OFF_OFF:
            assert(0);
            break;
        case BINGO_ANA_MICBIAS_OFF_ON:
            /* of of represents open micbias0 without open micbias1  */
            if (mic_adc_state.micbias_used_mask < BINGO_ANA_MICBIAS_OFF_ON) {
                bingo_ana_micbias_sel_vout(BINGO_ANA_MICBIAS_OFF_OFF);
            }
            break;
        case BINGO_ANA_MICBIAS_ON_OFF:
            if (mic_adc_state.micbias_used_mask < BINGO_ANA_MICBIAS_ON_OFF) {
                bingo_ana_micbias_sel_vout(BINGO_ANA_MICBIAS_ON_OFF);
            }
            break;
        case BINGO_ANA_MICBIAS_ON_ON:
            if (mic_adc_state.micbias_used_mask < BINGO_ANA_MICBIAS_ON_OFF) {
                bingo_ana_micbias_sel_vout(BINGO_ANA_MICBIAS_ON_OFF);
            }
            break;
        default:
            break;
    }
    uint32_t micbias_reg_value = bingo_ana_get_micbias_vout();
    DBGLOG_DRIVER_INFO("[AUDIO ADC] micbias on, current_reg_micbias:0x%x\n", micbias_reg_value);
    mic_adc_state.micbias_used_mask |= micbias_mask;

    return RET_OK;
}

uint8_t iot_mic_bias_switch_off(uint8_t mic_map)
{
    uint8_t micbias_mask = 0;

    /* if need switch */
    if (!(mic_adc_state.mic_used_mask & mic_map)) {
        return RET_BUSY;
    }
    mic_adc_state.mic_used_mask &= (~mic_map);
    mic_adc_state.mic_used_mask &= IOT_MIC_BITMAP_MAX;

    for (uint8_t i = 0; i < IOT_MIC_ADC_MAX; i++) {
        if (mic_adc_state.mic_used_mask & BIT(i)) {
            //mic0~2's micbias is put in the bit0~2,
            micbias_mask |= (uint8_t)BIT((BIT(i) & mic_adc_state.really_used_micbias_map) ? 1 : 0);
        }
    }

    switch (micbias_mask) {
        case BINGO_ANA_MICBIAS_OFF_OFF:
            if (mic_adc_state.micbias_used_mask > BINGO_ANA_MICBIAS_OFF_ON) {
                bingo_ana_micbias_sel_vout(BINGO_ANA_MICBIAS_OFF_OFF);
            }
            break;
        case BINGO_ANA_MICBIAS_OFF_ON:
            if (mic_adc_state.micbias_used_mask > BINGO_ANA_MICBIAS_OFF_ON) {
                bingo_ana_micbias_sel_vout(BINGO_ANA_MICBIAS_OFF_OFF);
            }
            break;
        case BINGO_ANA_MICBIAS_ON_OFF:
            break;
        case BINGO_ANA_MICBIAS_ON_ON:
            break;
        default:
            break;
    }
    uint32_t micbias_reg_value = bingo_ana_get_micbias_vout();
    DBGLOG_DRIVER_INFO("[AUDIO ADC] micbias off, current_reg_micbias:0x%x\n", micbias_reg_value);
    mic_adc_state.micbias_used_mask &= micbias_mask;

    return RET_OK;
}

uint8_t iot_mic_get_micbias_vout(void)
{
    return mic_adc_state.micbias_used_mask;
}

void iot_mic_bias_init(void)
{
    uint32_t mic_bias_trim_code;
    bingo_ana_micbias_current_limit_enable(true);
    bingo_ana_micbias_enable(BINGO_ANA_MICBIAS_FIRST_STAGE, true);
    bingo_ana_micbias_enable(BINGO_ANA_MICBIAS_SECOND_STAGE, true);
    bingo_ana_micbias_shield();
    bingo_ana_micbias_cap_comp_adjust();
    bingo_ana_micbias_sst_n(true);
    /*delay 200us for fast start up*/
    iot_timer_delay_us(200);
    bingo_ana_micbias_sst_n(false);
    mic_bias_trim_code = cal_data_trim_code_get(MICBIAS_TRIM_CODE);
    bingo_ana_micbias_ctrl((BINGO_ANA_MICBIAS_OUT)mic_bias_trim_code);
    bingo_ana_micbias_mode_ctrl(BINGO_ANA_MICBIAS_NORMAL_MODE);
    bingo_ana_micbias_sel_crrnt_flt(BINGO_ANA_MICBIAS_CUR_LOW_R);
}

static void meter_codec_ldo_en(bool_t enable)
{
    bingo_pmm_adc_ldo_enable(LDO_1P2_MODULE_MIC_ADC, enable);
}

void iot_mic_adc_vref_ctrl(IOT_MIC_ADC id, uint8_t vref_code)
{
    bingo_ana_mic_adc_vref_ctrl((BINGO_ANA_MIC_CHANNEL)id, vref_code);
}

void iot_mic_adc_vcm_ctrl(IOT_MIC_ADC chn, uint8_t vcm_code)
{
    bingo_ana_mic_vcm_ctrl((BINGO_ANA_MIC_CHANNEL)chn, vcm_code);
}

uint8_t iot_mic_reg_init(void)
{
    uint8_t mic_vref_trim_code[3] = {0};
    uint8_t mic_vcm_trim_code[3] = {0};
    /*enable ldo1p2*/
    meter_codec_ldo_en(true);
    iot_mic_bias_init();
    /*rc tuning before pga configure*/
    if (iot_mic_rc_tuning() != RET_OK) {
        return RET_INVAL;
    }
    // load cal data
    mic_vref_trim_code[0] = cal_data_trim_code_get(MIC_VREF_TRIM_CODE) & 0xff;
    mic_vref_trim_code[1] = (cal_data_trim_code_get(MIC_VREF_TRIM_CODE) >> 8) & 0xff;
    mic_vref_trim_code[2] = (cal_data_trim_code_get(MIC_VREF_TRIM_CODE) >> 16) & 0xff;

    mic_vcm_trim_code[0] = cal_data_trim_code_get(MIC_VCM_TRIM_CODE) & 0xff;
    mic_vcm_trim_code[1] = (cal_data_trim_code_get(MIC_VCM_TRIM_CODE) >> 8) & 0xff;
    mic_vcm_trim_code[2] = (cal_data_trim_code_get(MIC_VCM_TRIM_CODE) >> 16) & 0xff;

    iot_mic_adc_vref_ctrl(IOT_MIC_ADC_0, mic_vref_trim_code[0]);
    iot_mic_adc_vref_ctrl(IOT_MIC_ADC_1, mic_vref_trim_code[1]);
    iot_mic_adc_vref_ctrl(IOT_MIC_ADC_2, mic_vref_trim_code[2]);

    iot_mic_adc_vcm_ctrl(IOT_MIC_ADC_0, mic_vcm_trim_code[0]);
    iot_mic_adc_vcm_ctrl(IOT_MIC_ADC_1, mic_vcm_trim_code[1]);
    iot_mic_adc_vcm_ctrl(IOT_MIC_ADC_2, mic_vcm_trim_code[2]);
    bingo_ana_micbias_current_limit_enable(false);

    return RET_OK;
}

uint8_t iot_mic_config(uint8_t mic_map, iot_mic_adc_timer_done_callback cb)
{
    assert(mic_adc_state.mic_open_cnt < MIC_OPEN_CNT_MAX);

    cpu_critical_enter();
    mic_adc_state.mic_cb[mic_adc_state.mic_open_cnt] = cb;
    mic_adc_state.mic_open_map |= BIT(mic_adc_state.mic_open_cnt);
    mic_adc_state.current_mic_map[mic_adc_state.mic_open_cnt]= mic_map;
    mic_adc_state.mic_open_cnt++;
    cpu_critical_exit();

    iot_mic_adc_pga_init(mic_map);

    return RET_OK;
}

void iot_mic_bias_control(IOT_MIC_ADC_MICBIAS_OUT micbias_out)
{
    bingo_ana_micbias_ctrl((BINGO_ANA_MICBIAS_OUT)micbias_out);
}

void iot_mic_bias_mode(IOT_MIC_ADC_MICBIAS_MODE mode)
{
    bingo_ana_micbias_mode_ctrl((BINGO_ANA_MICBIAS_MODE)mode);
}

void iot_mic_gain_control(IOT_MIC_ADC id, IOT_MIC_ADC_GAIN gain)
{
    bingo_ana_mic_pga_gain_ctrl((BINGO_ANA_MIC_CHANNEL)id, (BINGO_ANA_MIC_GAIN)gain);
}

void iot_mic_low_power_mode(IOT_MIC_ADC id)
{
    BINGO_ANA_MIC_CHANNEL chid = (BINGO_ANA_MIC_CHANNEL)id;
    bingo_ana_mic_pga_bias_sel(chid, BINGO_ANA_HALF_HALF);
    bingo_ana_mic_adc_bias_int_ctrl(chid, BINGO_ANA_FISRT_INT, 0);
    bingo_ana_mic_adc_bias_int_ctrl(chid, BINGO_ANA_SECOND_INT, 0);
    bingo_ana_mic_adc_bias_vrefp_ctrl(chid, 0);
}

void iot_mic_release(uint8_t mic_map)
{
    for (uint8_t chid = 0; chid < (uint8_t)IOT_MIC_ADC_MAX; chid++) {
        if (mic_map & BIT(chid)) {
            bingo_ana_mic_adc_dem_enable((BINGO_ANA_MIC_CHANNEL)chid, false);
            bingo_ana_mic_adc_enable((BINGO_ANA_MIC_CHANNEL)chid, false);
            bingo_ana_mic_pga_enable((BINGO_ANA_MIC_CHANNEL)chid, false);
        }
    }
}

void iot_mic_bias_deinit(void)
{
    bingo_ana_micbias_enable(BINGO_ANA_MICBIAS_FIRST_STAGE, false);
    bingo_ana_micbias_enable(BINGO_ANA_MICBIAS_SECOND_STAGE, false);
}

void iot_mic_rc_tune_reset(void)
{
    bingo_ana_ct_sdm_rc_tune_target(0);
}
