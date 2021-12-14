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

#include "iot_irq.h"
#include "iot_charger.h"

#include "pmm.h"
#include "pmm_ana.h"
#include "iot_adc.h"
#include "caldata.h"
#include "driver_dbglog.h"

#define VBAT_ADC_SUM_AVERAGE_NUM 1

static iot_charger_int_callback iot_charger_int_cb = NULL;
static bool_t iot_charger_inited = false;
static iot_irq_t iot_charger_irq = 0;
static cal_data_charger iot_charger_cal_data = {0};
static bool_t iot_charger_gpio_en = false;

static const int16_t def_vol[32] = {
    3734, 3772, 3802, 3832, 3863, 3894, 3925, 3857, 3990, 4018, 4046, 4074, 4103, 4132, 4162, 4192,
    4222, 4253, 4284, 4316, 4348, 4381, 4415, 4448, 4483, 4510, 4539, 4567, 4596, 4625, 4655, 4685};

static uint32_t iot_charger_isr_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_charger_isr_handler);
static uint32_t iot_charger_isr_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);

    uint8_t flag = pmm_charger_flag_get();

    if (iot_charger_int_cb) {
        iot_charger_int_cb(flag);
    }

    pmm_charger_flag_int_clear();

    return 0;
}

static void iot_charger_int_config(void)
{
    iot_charger_irq = iot_irq_create(CHARGER_ON_FLAG_INT, 0, iot_charger_isr_handler);
}

void iot_charger_init(void)
{
    if (!iot_charger_inited) {

        uint8_t pmos_code = (uint8_t)cal_data_trim_code_get(CHG_IOUT_STEP_TRIM_CODE);
        uint8_t flag = pmm_charger_flag_get();

        cal_data_charger_get(&iot_charger_cal_data);

        iot_charger_int_cb = NULL;
        pmm_charger_flag_en(true);
        pmm_ana_charger_iout_pmos_cal(pmos_code);
        iot_charger_int_config();

        // close chg pin uart if in box
        if (iot_charger_gpio_en) {
            if (flag) {
                pmm_charger_vbus_gpio_enable(false);
            } else {
                pmm_charger_vbus_gpio_enable(true);
            }
        }
        pmm_charger_uvp_sel();

        iot_charger_inited = true;
    }
}

bool_t iot_charger_flag_get(void)
{
    return !!pmm_charger_flag_get();
}

void iot_charger_register_int_cb(IOT_CHARGER_INT_TYPE int_type, iot_charger_int_callback cb)
{
    pmm_charger_flag_int_type((PMM_CHG_INT_MODE)int_type);
    iot_charger_int_cb = cb;

    iot_irq_unmask(iot_charger_irq);
}

void iot_charger_gpio_enable(bool_t enable)
{
    iot_charger_gpio_en = enable;
    pmm_charger_vbus_gpio_enable(enable);
}

void iot_charger_set_current(uint16_t cur_0_1ma)
{
    pmm_set_charger_current(cur_0_1ma);
}

void iot_charger_set_current_dp(uint16_t cur_0_1ma)
{
    pmm_set_dp_charger_current(cur_0_1ma);
}

void iot_charger_set_voltage(IOT_CHARGER_VOL vol)
{
    int8_t vout_code;
    float ratio;
    float delta_vol;

    assert(iot_charger_inited);

    delta_vol = iot_charger_cal_data.chg_vout_v4p3_mv - iot_charger_cal_data.chg_vout_v4p0_mv;
    ratio = ((delta_vol)*1.0f) / CHG_VOUT_DELTA_300MV_CODE;
    vout_code = (int8_t)((def_vol[vol] - iot_charger_cal_data.chg_vout_v4p0_mv) / ratio)
        + CHG_VOUT_4000MV_CODE;
    if (vout_code < CHG_VOUT_MIN_CODE) {
        vout_code = CHG_VOUT_MIN_CODE;
    } else if (vout_code > CHG_VOUT_MAX_CODE) {
        vout_code = CHG_VOUT_MAX_CODE;
    }

    pmm_ana_set_charger_voltage((uint8_t)vout_code);
}

void iot_charger_int_enable(void)
{
    pmm_charger_flag_int_en(true);
}

void iot_charger_clear_charger_flag_int(void)
{
    pmm_charger_flag_int_clear();
}

void iot_charger_int_disable(void) IRAM_TEXT(iot_charger_int_disable);
void iot_charger_int_disable(void)
{
    pmm_charger_flag_int_en(false);
}

IOT_CHARGER_INT_TYPE iot_charger_get_int_type(void)
{
    return (IOT_CHARGER_INT_TYPE)pmm_get_charger_flag_int_type();
}

uint16_t iot_charger_get_vbat_mv(void)
{
    float sig_mv;

    assert(iot_charger_inited);

    int32_t adc_code = iot_adc_poll_data(IOT_ADC_CHARGER_OUT_VOLTAGE, 0, VBAT_ADC_SUM_AVERAGE_NUM);
    sig_mv = iot_adc_2_mv(0, adc_code);

    sig_mv = sig_mv / iot_charger_cal_data.chg_vout_madc_ratio;

    return (uint16_t)sig_mv;
}

uint16_t iot_charger_get_vcharger_mv(void)
{
    float sig_mv;

    assert(iot_charger_inited);

    int32_t adc_code = iot_adc_poll_data(IOT_ADC_CHARGER_IN_VOLTAGE, 0, VBAT_ADC_SUM_AVERAGE_NUM);
    sig_mv = iot_adc_2_mv(0, adc_code);

    sig_mv = sig_mv / 0.147f;

    return (uint16_t)sig_mv;
}

void iot_charger_mon_enable(void)
{
    pmm_ana_charger_cur_mon_en(true);
}

bool_t iot_charger_get_charger_mon_flag(void)
{
    return pmm_ana_get_charger_cur_mon_flag();
}

void iot_charger_clear_charger_mon_flag(void)
{
    pmm_ana_charger_cur_mon_en(false);
}

uint8_t iot_charger_get_charger_state(void)
{
    return (uint8_t)pmm_ana_get_charger_state();
}
