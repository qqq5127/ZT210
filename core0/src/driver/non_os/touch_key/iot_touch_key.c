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

#include "types.h"

#include "gpio.h"
#include "pin.h"
#include "caldata.h"

#include "iot_irq.h"
#include "iot_timer.h"
#include "touch_key.h"
#include "iot_touch_key.h"

#ifdef IOT_TOUCK_KEY_DEBUG
#include "pmm.h"
#include "driver_dbglog.h"
#endif

#define GPIO_TOUCH_KEY_START GPIO_67

typedef struct iot_touch_key_int {
    iot_touch_key_callback cb;
    uint32_t cur_cdc;
    uint8_t phase_state;
    IOT_TK_PAD_ID pad_id;
    IOT_TK_WORK_MODE work_mode;
    TK_PHASE_INT int_mode;
} iot_touch_key_int_t;

static uint8_t g_touch_id_num = 0;
static iot_irq_t touch_key_irq = 0;
static iot_touch_key_pad_info_t pad_info[IOT_TK_PAD_ID_2] = {
    {IOT_TK_PAD_ID_INVALID, IOT_TK_NO_MODE, {IOT_TOUCH_KEY_PHASE_NUM_INVALID}},
    {IOT_TK_PAD_ID_INVALID, IOT_TK_NO_MODE, {IOT_TOUCH_KEY_PHASE_NUM_INVALID}}};
static iot_touch_key_int_t iot_touch_key_info[IOT_TK_PHASE_ID_MAX] = {0};

static uint32_t touch_key_isr_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(touch_key_isr_handler);
static uint32_t touch_key_isr_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);
    IOT_TK_PHASE_ID phase_id;
    TK_PHASE_INT tk_int_st;
    uint8_t total_phase_num = iot_touch_key_get_phase_num();

    for (phase_id = IOT_TK_PHASE_ID_0; phase_id < total_phase_num; phase_id++) {
        tk_int_st = touch_key_get_xphase_int_st((TK_PHASE_ID)phase_id);
        if (!tk_int_st) {
            continue;
        }

        if (tk_int_st == TK_PHASE_CLIMB_INT) {
            iot_touch_key_info[phase_id].cur_cdc = touch_key_phase_read_cdc((TK_PHASE_ID)phase_id)
                >> touch_key_read_phase_aver_num((TK_PHASE_ID)phase_id);
            iot_touch_key_info[phase_id].int_mode = TK_PHASE_CLIMB_INT;
            touch_key_clr_xphase_intr((TK_PHASE_ID)phase_id, true, false);
        } else {
            iot_touch_key_info[phase_id].int_mode = TK_PHASE_FALL_INT;
            touch_key_clr_xphase_intr((TK_PHASE_ID)phase_id, false, true);
        }

        if (iot_touch_key_info[phase_id].cb) {
            iot_touch_key_info[phase_id].cb(iot_touch_key_info[phase_id].pad_id,
                                            (IOT_TOUCH_KEY_INT)tk_int_st);
        }
    }
    return 0;
}

static void touch_key_irq_init(void)
{
    uint32_t touch_key_irq_vector = touch_key_get_irq_vector();

    /* register touch_key irq */
    touch_key_irq = iot_irq_create(touch_key_irq_vector, 0, touch_key_isr_handler);

    /* turn on touch_key's irq */
    iot_irq_unmask(touch_key_irq);
    //iot_irq_mask(touch_key_irq);
}

static void touch_key_irq_deinit(void)
{
    /* turn off touch_key's irq */
    iot_irq_mask(touch_key_irq);

    /* release irq resource */
    iot_irq_delete(touch_key_irq);

    touch_key_irq = 0;
}

uint8_t iot_touch_key_reset_pad(IOT_TK_PAD_ID pad_id)
{
    uint8_t ret;
    IOT_TK_PHASE_ID phase_id;

    ret = iot_touch_key_get_work_phase(pad_id, &phase_id);
    if (ret != RET_OK) {
        return ret;
    }
    touch_key_reset_xphase((TK_PHASE_ID)phase_id);

    return RET_OK;
}

void iot_touch_key_vref_ctrl_cfg(uint32_t vref_ctrl)
{
    touch_key_vref_ctrl_cfg(vref_ctrl);
}

static void iot_touch_key_digtal_init(void)
{
    touch_key_set_basic_param();
    touch_key_set_set_cdc_sample_thrs(8);
}

static void iot_touch_key_ldo_on(void)
{
    touch_key_enable_ana_ldo1();
    //delay 150us
    iot_timer_delay_us(150);

    touch_key_enable_ana_ldo2();
    //delay 300us
    iot_timer_delay_us(300);

    touch_key_set_ana_ldo_lpf_res_sel();
}

static void touch_key_power_init(void)
{
    uint32_t ldo_ctrl;
    ldo_ctrl = cal_data_trim_code_get(TK_LDO_TRIM_CODE);
    touch_key_ana_ldo_trim(ldo_ctrl);
    touch_key_ldo_stability_ctl();
    touch_key_ana_ldo_lp_mode_en(1);
    iot_touch_key_ldo_on();
}

static void iot_touch_key_analog_init(void)
{
    uint32_t verf_ctrl;
    touch_key_clk_init();
    touch_key_power_init();
    //The high eight represents high sensitivity and the low eight represents low sensitivity
    verf_ctrl = cal_data_trim_code_get(TK_VREF_TRIM_CODE) >> 8;
    touch_key_set_ana_circuit_enable(verf_ctrl);
}

void iot_touch_key_adjust_freq(IOT_TOUCH_KEY_DIV_FREQ freq)
{
    touch_key_adjust_freq((TOUCH_KEY_DIV_FREQ)freq);
}

void iot_touch_key_set_phase_enable(void)
{
    uint8_t phase_num;
    uint8_t phase_point_sum;
    uint8_t set_phase_num = IOT_TK_PHASE_ID_MAX;
    IOT_TK_PAD_ID pad_id[2] = {IOT_TK_PAD_ID_INVALID};
    uint8_t work_point_num[2] = {0};
    uint8_t monitor_point_num[2] = {0};

    for (uint8_t i = 0; i < IOT_TK_PAD_ID_2; i++) {
        pad_id[i] = pad_info[i].pad_id;
        work_point_num[i] = pad_info[i].point_num.work_point_num;
        monitor_point_num[i] = pad_info[i].point_num.monitor_point_num;
    }

    if (g_touch_id_num == 1) {
        if (pad_info[0].work_mode == IOT_TK_ABSOLUTE_MODE) {
            monitor_point_num[0] = pad_info[0].point_num.monitor_point_num + 2;
            work_point_num[0] = pad_info[0].point_num.work_point_num + 1;
            set_phase_num = 2;   //only inear feature
        } else if (pad_info[0].work_mode == IOT_TK_RELATIVE_MODE) {
            set_phase_num = 2;
        }
    }

    for (phase_num = IOT_TK_PHASE_ID_0; phase_num < set_phase_num; phase_num++) {

        //choose default param or set param
        if (!work_point_num[0] && !monitor_point_num[0]) {
            phase_point_sum = IOT_TOUCH_KEY_PHASE_POINT_NUM_5;
        } else {
            if (!(phase_num % 2)) {
                phase_point_sum = monitor_point_num[phase_num / 2];
            } else {
                phase_point_sum = work_point_num[phase_num / 2];
            }
        }
        touch_key_set_xphase_init_config((TK_PHASE_ID)phase_num, true, phase_point_sum);
        touch_key_set_pad_xphase_cfg0((TK_PHASE_ID)phase_num, (TK_PAD_ID)pad_id[phase_num / 2]);
        iot_touch_key_info[phase_num].phase_state = IOT_TK_PHASE_IDLE;
        iot_touch_key_info[phase_num].pad_id = pad_id[phase_num / 2];
    }
    touch_key_set_pad_enable_all();
    touch_key_set_work_phase_num(set_phase_num);
}

void iot_touch_key_set_phase_dummy_config(IOT_TK_PHASE_ID phase_id, bool_t dummy_ena)
{
    IOT_TK_PHASE_ID work_phase = phase_id;
    touch_key_set_xphase_rst((TK_PHASE_ID)work_phase);
    touch_key_set_xphase_cfg0_dummy_mode((TK_PHASE_ID)work_phase, dummy_ena);
    touch_key_release_xphase_rst((TK_PHASE_ID)work_phase);
}

void iot_touch_key_init()
{
}

void iot_touch_key_set_pad_info(IOT_TK_PAD_ID pad_id, IOT_TK_WORK_MODE work_mode,
                                const iot_touch_key_point_num_t *point_num)
{
    assert(pad_id < IOT_TK_PAD_ID_MAX && pad_id >= IOT_TK_PAD_ID_0);
    pad_info[g_touch_id_num].pad_id = pad_id;
    pad_info[g_touch_id_num].work_mode = work_mode;
    pad_info[g_touch_id_num].point_num.monitor_point_num = point_num->monitor_point_num;
    pad_info[g_touch_id_num].point_num.work_point_num = point_num->work_point_num;
    g_touch_id_num++;
}

void iot_touch_key_work_enable_with_phase(void)
{
    iot_touch_key_analog_init();
    iot_touch_key_digtal_init();
    touch_key_annalog_pad_enable(true);
    iot_touch_key_set_phase_enable();

    /* IRQ attach*/
    touch_key_irq_init();

    /* touch_key start work*/
    touch_key_start_work();
}

void iot_touch_key_work_enable(void)
{
    iot_touch_key_analog_init();
    iot_touch_key_digtal_init();
    touch_key_annalog_pad_enable(true);

    /* IRQ attach*/
    touch_key_irq_init();

    /* touch_key start work*/
    touch_key_start_work();
}

void iot_touch_key_disable_work(void)
{
    touch_key_soft_rst();
    touch_key_clk_deinit();

    /* IRQ deattach */
    touch_key_irq_deinit();
    touch_key_stop_work();
}

void iot_touch_key_deinit(void)
{
}

uint8_t iot_touch_key_bind_pad_for_work_phase(IOT_TK_PAD_ID pad_id, IOT_TK_WORK_MODE work_mode,
                                              iot_touch_key_phase_mode_t *phase_ptr,
                                              const iot_touch_key_config_t *set_param,
                                              iot_touch_key_callback cb)
{
    bool_t dummy_ena = false;
    iot_touch_key_phase_mode_t *phase_id = phase_ptr;
    IOT_TK_PHASE_ID work_phase;
    uint32_t sample_value_sel = IOT_TOUCH_KEY_SAMPLE_POINT_VALUE_SUM;
    uint32_t stragegy_sel = IOT_TOUCH_KEY_RELATIVE_THRS_SEL;
    uint8_t pre_point = IOT_TOUCH_KEY_RELA_PRE_POINT;
    uint8_t climb_trig_times;
    uint8_t fall_trig_times;
    uint32_t fall_thrs;
    uint32_t climb_thrs;

    UNUSED(pad_id);

    if (phase_id != NULL) {
        work_phase = (IOT_TK_PHASE_ID)phase_id->work_phase;
    } else {
        return RET_INVAL;
    }

    //choose default param or set param
    if (!set_param->climb_trig_times && !set_param->fall_trig_times) {
        climb_trig_times = IOT_TOUCH_KEY_DEFAULT_CLIMB_TRIG_TIMES;
        fall_trig_times = IOT_TOUCH_KEY_DEFAULT_FALL_TRIG_TIMES;
    } else {
        climb_trig_times = set_param->climb_trig_times;
        fall_trig_times = set_param->fall_trig_times;
    }

    if (!set_param->fall_thrs && !set_param->climb_thrs) {
        fall_thrs = IOT_TOUCH_KEY_TOUCH_FALL_THRS;
        climb_thrs = IOT_TOUCH_KEY_TOUCH_CLIMB_THRS;
    } else {
        fall_thrs = set_param->fall_thrs;
        climb_thrs = set_param->climb_thrs;
    }

    if (work_mode == IOT_TK_RELATIVE_MODE) {
        sample_value_sel = IOT_TOUCH_KEY_SAMPLE_POINT_VALUE_SUM;
        stragegy_sel = IOT_TOUCH_KEY_RELATIVE_THRS_SEL;
        pre_point = IOT_TOUCH_KEY_RELA_PRE_POINT;
    } else if (work_mode == IOT_TK_ABSOLUTE_MODE) {
        sample_value_sel = IOT_TOUCH_KEY_SAMPLE_POINT_VALUE_AVER;
        stragegy_sel = IOT_TOUCH_KEY_ABSOLUTE_THRS_SEL;
        pre_point = IOT_TOUCH_KEY_ABS_MODE_PRE_POINT;
    }

    touch_key_set_xphase_rst((TK_PHASE_ID)work_phase);

    //set phase param
    touch_key_set_xphase_cfg0_without_pad((TK_PHASE_ID)work_phase, dummy_ena, sample_value_sel,
                                          stragegy_sel, pre_point);
    //set thrs
    touch_key_set_xphase_cfg3_change_thrs((TK_PHASE_ID)work_phase, fall_thrs, climb_thrs);
    //set trig times
    touch_key_set_xphase_trig_times((TK_PHASE_ID)work_phase, climb_trig_times, fall_trig_times);

    touch_key_release_xphase_rst((TK_PHASE_ID)work_phase);

    iot_touch_key_info[work_phase].work_mode = work_mode;
    iot_touch_key_info[work_phase].cb = cb;
    iot_touch_key_info[work_phase].phase_state = IOT_TK_PHASE_BUSY;

    return RET_OK;
}

uint8_t iot_touch_key_bind_pad_for_monitor_phase(IOT_TK_PAD_ID pad_id, IOT_TK_WORK_MODE work_mode,
                                                 iot_touch_key_phase_mode_t *phase_ptr,
                                                 iot_touch_key_callback cb)
{
    bool_t dummy_ena = false;
    uint32_t sample_value_sel = IOT_TOUCH_KEY_SAMPLE_POINT_VALUE_AVER;
    uint32_t stragegy_sel = IOT_TOUCH_KEY_ABSOLUTE_THRS_SEL;
    uint32_t fall_thrs = IOT_TOUCH_KEY_MAX_THRS;
    uint32_t climb_thrs = IOT_TOUCH_KEY_MAX_THRS;
    uint8_t pre_point = IOT_TOUCH_KEY_ABS_MODE_PRE_POINT;
    bool_t enable = false;
    IOT_TK_PHASE_ID monitor_phase;
    iot_touch_key_phase_mode_t *phase_id = phase_ptr;

    UNUSED(pad_id);

    if (phase_id != NULL) {
        monitor_phase = (IOT_TK_PHASE_ID)phase_id->monitor_phase;
    } else {
        return RET_INVAL;
    }

    touch_key_set_xphase_rst((TK_PHASE_ID)monitor_phase);

    //set phase param
    touch_key_set_xphase_cfg0_without_pad((TK_PHASE_ID)monitor_phase, dummy_ena, sample_value_sel,
                                          stragegy_sel, pre_point);
    //set ear_intr thrs
    touch_key_set_xphase_cfg3_change_thrs((TK_PHASE_ID)monitor_phase, fall_thrs, climb_thrs);
    //enable intr
    touch_key_set_xphase_cfg1((TK_PHASE_ID)monitor_phase, enable);

    touch_key_release_xphase_rst((TK_PHASE_ID)monitor_phase);

    iot_touch_key_info[monitor_phase].work_mode = work_mode;
    iot_touch_key_info[monitor_phase].cb = cb;
    iot_touch_key_info[monitor_phase].phase_state = IOT_TK_PHASE_BUSY;

    return RET_OK;
}

uint8_t iot_touch_key_change_pad_thrs(IOT_TK_PAD_ID pad_id, uint32_t fall_thrs, uint32_t climb_thrs)
{
    uint8_t ret;
    IOT_TK_PHASE_ID work_phase;

    ret = iot_touch_key_get_work_phase(pad_id, &work_phase);
    if (ret != RET_OK) {
        return ret;
    }
    touch_key_set_xphase_cfg3_change_thrs((TK_PHASE_ID)work_phase, fall_thrs, climb_thrs);
    return RET_OK;
}

uint8_t iot_touch_key_change_pad_trig_times(IOT_TK_PAD_ID pad_id, uint8_t climb_trig_times,
                                            uint8_t fall_trig_times)
{
    uint8_t ret;
    IOT_TK_PHASE_ID work_phase;

    ret = iot_touch_key_get_work_phase(pad_id, &work_phase);
    if (ret != RET_OK) {
        return ret;
    }

    touch_key_set_xphase_trig_times((TK_PHASE_ID)work_phase, climb_trig_times, fall_trig_times);
    return RET_OK;
}

uint8_t iot_touch_key_check_idle_phase(IOT_TK_PHASE_ID start_phase, IOT_TK_PHASE_ID *phase_id)
{
    IOT_TK_PHASE_ID phase_num;

    for (phase_num = start_phase; phase_num < IOT_TK_PHASE_ID_MAX; phase_num++) {
        if (touch_key_check_dummy_phase((TK_PHASE_ID)phase_num)
            && !touch_key_check_work_phase((TK_PHASE_ID)phase_num)) {
            *phase_id = phase_num;
            return RET_OK;
        }
    }

    return RET_NOT_EXIST;
}

uint8_t iot_touch_key_get_two_idle_phase(iot_touch_key_phase_mode_t *phase_id)
{
    uint8_t ret;
    IOT_TK_PHASE_ID work_phase;
    IOT_TK_PHASE_ID monitor_phase;

    ret = iot_touch_key_check_idle_phase(IOT_TK_PHASE_ID_0, &monitor_phase);
    if (ret != RET_OK) {
        return ret;
    }

    ret = iot_touch_key_check_idle_phase((IOT_TK_PHASE_ID)(monitor_phase + 1), &work_phase);
    if (ret != RET_OK) {
        return ret;
    }

    (*phase_id).monitor_phase = monitor_phase;
    (*phase_id).work_phase = work_phase;

    return RET_OK;
}

uint8_t iot_touch_key_get_phase_num(void)
{
    return (uint8_t)touch_key_read_work_phase_num();
}

uint8_t iot_touch_key_get_pad_all_phase(IOT_TK_PAD_ID pad_id, iot_touch_key_phase_mode_t *phase_id)
{
    uint8_t ret;
    IOT_TK_PHASE_ID work_phase;
    IOT_TK_PHASE_ID monitor_phase;

    ret = iot_touch_key_get_pad_phase(pad_id, IOT_TK_PHASE_ID_0, &monitor_phase);
    if (ret != RET_OK) {
        return ret;
    }
    ret = iot_touch_key_get_pad_phase(pad_id, monitor_phase + IOT_TK_PHASE_ID_1, &work_phase);
    if (ret != RET_OK) {
        return ret;
    }

    (*phase_id).monitor_phase = monitor_phase;
    (*phase_id).work_phase = work_phase;
    return RET_OK;
}

uint8_t iot_touch_key_read_pad_used(IOT_TK_PAD_ID pad_id, IOT_TK_WORK_MODE work_mode,
                                    iot_touch_key_callback cb)
{
    uint8_t ret = RET_OK;
    IOT_TK_PHASE_ID i;
    uint8_t total_phase_num;
    total_phase_num = iot_touch_key_get_phase_num();

    for (i = IOT_TK_PHASE_ID_0; i < total_phase_num; i++) {
        if (pad_id == touch_key_read_phase_used_pad((TK_PHASE_ID)i)
            && (!touch_key_check_dummy_phase((TK_PHASE_ID)i))) {
            //wake up and open again to assign callback
            iot_touch_key_info[i].cb = cb;
            iot_touch_key_info[i].work_mode = work_mode;
            iot_touch_key_info[i].phase_state = IOT_TK_PHASE_BUSY;
            iot_touch_key_info[i].pad_id = pad_id;
            ret = RET_EXIST;
        }
    }
    return ret;
}

void iot_touch_key_set_use_num(uint8_t touch_id_num)
{
    g_touch_id_num = touch_id_num;
}

uint8_t iot_touch_key_enable_intr(IOT_TK_PAD_ID pad_id)
{
    uint8_t ret;
    IOT_TK_PHASE_ID work_phase;

    ret = iot_touch_key_get_work_phase_no_intr(pad_id, &work_phase);
    if (ret != RET_OK) {
        return ret;
    }
    if (!touch_key_check_work_phase((TK_PHASE_ID)work_phase)) {
        touch_key_set_xphase_rst((TK_PHASE_ID)work_phase);
        touch_key_set_xphase_cfg1((TK_PHASE_ID)work_phase, true);
        touch_key_release_xphase_rst((TK_PHASE_ID)work_phase);
    }
    return RET_OK;
}

uint8_t iot_touch_key_open(IOT_TK_PAD_ID pad_id, IOT_TK_WORK_MODE work_mode,
                           const iot_touch_key_config_t *param, iot_touch_key_callback cb)
{
    uint8_t ret;
    iot_touch_key_phase_mode_t phase_id;

    ret = (uint8_t)pin_claim_as_gpio(pad_id + GPIO_TOUCH_KEY_START);
    if (ret != RET_OK) {
        return ret;
    }

    if (!touch_key_read_start_work()) {
        iot_touch_key_work_enable_with_phase();
    }

    ret = iot_touch_key_read_pad_used(pad_id, work_mode, cb);
    if (ret != RET_OK) {
        touch_key_irq_init();
        return RET_OK;
    }

    ret = iot_touch_key_get_pad_all_phase(pad_id, &phase_id);
    if (ret != RET_OK) {
        pin_release(pad_id + GPIO_TOUCH_KEY_START);
        return ret;
    }

    ret = iot_touch_key_bind_pad_for_monitor_phase(pad_id, work_mode, &phase_id, cb);
    if (ret != RET_OK) {
        pin_release(pad_id + GPIO_TOUCH_KEY_START);
        return ret;
    }

    ret = iot_touch_key_bind_pad_for_work_phase(pad_id, work_mode, &phase_id, param, cb);
    if (ret != RET_OK) {
        pin_release(pad_id + GPIO_TOUCH_KEY_START);
        return ret;
    }

    return RET_OK;
}

uint8_t iot_touch_key_close(IOT_TK_PAD_ID pad_id)
{
    uint8_t ret;
    uint8_t total_phase_num = iot_touch_key_get_phase_num();

    ret = iot_touch_key_close_pad(pad_id);
    if (ret != RET_OK) {
        return ret;
    }

    pin_release(pad_id + GPIO_TOUCH_KEY_START);

    if (iot_touch_key_get_dummy_phase_num() == total_phase_num) {
        iot_touch_key_disable_work();
    }

    return RET_OK;
}

uint8_t iot_touch_key_get_phase_arr(IOT_TK_PHASE_ID phase_id, IOT_TK_PHASE_ID *phase_arr,
                                    uint8_t *total_phase_num)
{
    IOT_TK_PHASE_ID i;
    uint8_t phase_num = 0;

    IOT_TK_PAD_ID pad_id = iot_touch_key_info[phase_id].pad_id;

    //save same touch_type phase & pad_id  into array
    for (i = IOT_TK_PHASE_ID_0; i < IOT_TK_PHASE_ID_MAX; i++) {
        if ((iot_touch_key_info[i].pad_id == pad_id)) {
            *(phase_arr + phase_num) = i;
            phase_num++;
        }
    }

    if (!phase_num) {
        return RET_NOT_EXIST;
    }

    *total_phase_num = phase_num;
    return RET_OK;
}

uint8_t iot_touch_key_close_pad(IOT_TK_PAD_ID pad_id)
{
    IOT_TK_PHASE_ID close_phase[IOT_TK_PHASE_ID_MAX] = {IOT_TK_PHASE_ID_0};
    IOT_TK_PHASE_ID phase_num = IOT_TK_PHASE_ID_0;
    IOT_TK_PHASE_ID i;
    IOT_TK_PHASE_ID work_phase;

    for (i = IOT_TK_PHASE_ID_0; i < IOT_TK_PHASE_ID_MAX; i++) {
        if ((!touch_key_check_dummy_phase((TK_PHASE_ID)i))
            && (touch_key_read_phase_used_pad((TK_PHASE_ID)i) == pad_id)) {
            close_phase[phase_num] = i;
            phase_num++;
        }
    }

    if (!phase_num) {
        return RET_NOT_EXIST;
    }

    for (i = IOT_TK_PHASE_ID_0; i < phase_num; i++) {
        work_phase = close_phase[i];
        touch_key_set_xphase_rst((TK_PHASE_ID)work_phase);
        //set into dummy phase
        touch_key_set_xphase_cfg0_dummy_mode((TK_PHASE_ID)work_phase, true);
        //touch_key_set_pad_unuse(pad_id);
        //close int
        touch_key_set_xphase_cfg1((TK_PHASE_ID)work_phase, false);
        //unbind pad
        touch_key_set_pad_xphase_cfg0((TK_PHASE_ID)work_phase, (TK_PAD_ID)0xf);

        touch_key_release_xphase_rst((TK_PHASE_ID)work_phase);

        iot_touch_key_info[work_phase].phase_state = IOT_TK_PHASE_IDLE;
        iot_touch_key_info[work_phase].cb = NULL;
        iot_touch_key_info[work_phase].pad_id = (IOT_TK_PAD_ID)0xf;
        iot_touch_key_info[work_phase].int_mode = TK_PHASE_NO_INT;
    }
    return RET_OK;
}

uint8_t iot_touch_key_get_work_phase(IOT_TK_PAD_ID pad_id, IOT_TK_PHASE_ID *phase_ptr)
{
    IOT_TK_PHASE_ID i;
    uint8_t total_phase_num = iot_touch_key_get_phase_num();

    for (i = IOT_TK_PHASE_ID_0; i < total_phase_num; i++) {
        if ((!touch_key_check_dummy_phase((TK_PHASE_ID)i))
            && touch_key_check_work_phase((TK_PHASE_ID)i)
            && (pad_id == touch_key_read_phase_used_pad((TK_PHASE_ID)i))) {
            *phase_ptr = i;
            return RET_OK;
        }
    }

    return RET_NOT_EXIST;
}

uint8_t iot_touch_key_get_work_phase_no_intr(IOT_TK_PAD_ID pad_id, IOT_TK_PHASE_ID *phase_ptr)
{
    IOT_TK_PHASE_ID i;
    IOT_TK_PHASE_ID moniotr_phase = IOT_TK_PHASE_ID_0;
    uint8_t total_phase_num = iot_touch_key_get_phase_num();

    for (i = IOT_TK_PHASE_ID_0; i < total_phase_num; i++) {
        if ((!touch_key_check_dummy_phase((TK_PHASE_ID)i))
            && (pad_id == touch_key_read_phase_used_pad((TK_PHASE_ID)i))) {
            moniotr_phase = i;
            *phase_ptr = moniotr_phase + IOT_TK_PHASE_ID_1;
            return RET_OK;
        }
    }

    return RET_NOT_EXIST;
}

uint8_t iot_touch_key_get_pad_phase(IOT_TK_PAD_ID pad_id, IOT_TK_PHASE_ID start_phase,
                                    IOT_TK_PHASE_ID *phase_ptr)
{
    IOT_TK_PHASE_ID i;
    uint8_t total_phase_num = iot_touch_key_get_phase_num();

    for (i = start_phase; i < total_phase_num; i++) {
        if ((touch_key_check_dummy_phase((TK_PHASE_ID)i))
            && (pad_id == touch_key_read_phase_used_pad((TK_PHASE_ID)i))) {
            *phase_ptr = i;
            return RET_OK;
        }
    }

    return RET_NOT_EXIST;
}

uint8_t iot_touch_key_get_monitor_phase(IOT_TK_PAD_ID pad_id, IOT_TK_PHASE_ID *phase_ptr)
{
    IOT_TK_PHASE_ID i;
    uint8_t total_phase_num = iot_touch_key_get_phase_num();

    for (i = IOT_TK_PHASE_ID_0; i < total_phase_num; i++) {
        if ((!touch_key_check_dummy_phase((TK_PHASE_ID)i))
            && (!touch_key_check_work_phase((TK_PHASE_ID)i))
            && (pad_id == touch_key_read_phase_used_pad((TK_PHASE_ID)i))) {
            *phase_ptr = i;
            return RET_OK;
        }
    }

    return RET_NOT_EXIST;
}

uint8_t iot_touch_key_get_dummy_phase_num(void)
{
    uint8_t phase_num = 0;
    IOT_TK_PHASE_ID i;
    uint8_t total_phase_num = iot_touch_key_get_phase_num();

    for (i = IOT_TK_PHASE_ID_0; i < total_phase_num; i++) {
        if ((touch_key_check_dummy_phase((TK_PHASE_ID)i))) {
            phase_num++;
        }
    }

    return phase_num;
}

uint8_t iot_touch_key_read_pad_cdc(IOT_TK_PAD_ID pad_id, uint32_t *cdc_value)
{
    uint8_t ret;
    IOT_TK_PHASE_ID monitor_phase;
    uint32_t cdc;

    ret = iot_touch_key_get_monitor_phase(pad_id, &monitor_phase);
    if (ret != RET_OK) {
        return ret;
    }

    cdc = iot_touch_key_read_phase_nodelay_cdc(monitor_phase);

    *cdc_value = cdc;
    return RET_OK;
}

uint8_t iot_touch_key_check_pad_state(IOT_TK_PAD_ID pad_id, IOT_TOUCH_KEY_INT *pad_state)
{
    uint8_t ret;
    uint32_t cdc;
    IOT_TOUCH_KEY_INT pad_status = IOT_TOUCH_KEY_NO_INT;
    uint32_t diff;
    IOT_TK_PHASE_ID monitor_phase;
    IOT_TK_PHASE_ID work_phase;

    ret = iot_touch_key_get_monitor_phase(pad_id, &monitor_phase);
    if (ret != RET_OK) {
        return ret;
    }

    ret = iot_touch_key_get_work_phase(pad_id, &work_phase);
    if (ret != RET_OK) {
        return ret;
    }

    cdc = iot_touch_key_read_phase_nodelay_cdc(monitor_phase);
    if (!iot_touch_key_info[work_phase].cur_cdc) {
        pad_status = IOT_TOUCH_KEY_INT_PRESS_RELEASE;
    } else if (cdc <= iot_touch_key_info[work_phase].cur_cdc) {
        pad_status = IOT_TOUCH_KEY_INT_PRESS_RELEASE;
    } else if (cdc > iot_touch_key_info[work_phase].cur_cdc) {
        diff = cdc - iot_touch_key_info[work_phase].cur_cdc;
        if (diff < IOT_TOUCH_KEY_TOUCH_PHASE_TOLERAN) {
            pad_status = IOT_TOUCH_KEY_INT_PRESS_RELEASE;
        } else {
            pad_status = IOT_TOUCH_KEY_INT_PRESS_MID;
        }
    }

    if (pad_status == IOT_TOUCH_KEY_INT_PRESS_RELEASE
        && iot_touch_key_info[work_phase].int_mode == TK_PHASE_CLIMB_INT) {
        touch_key_reset_xphase((TK_PHASE_ID)work_phase);
        iot_touch_key_info[work_phase].int_mode = TK_PHASE_FALL_INT;
    }

    *pad_state = pad_status;
    return RET_OK;
}

uint32_t iot_touch_key_read_phase_nodelay_cdc(IOT_TK_PHASE_ID id)
{
    uint32_t cdc_std;
    uint32_t cdc_out;

    cdc_std = touch_key_phase_read_cdc((TK_PHASE_ID)id);
    cdc_out = touch_key_phase_read_cdc((TK_PHASE_ID)id);
    while (cdc_std != cdc_out) {
        cdc_std = cdc_out;
        cdc_out = touch_key_phase_read_cdc((TK_PHASE_ID)id);
    }
    return cdc_out;
}

#ifdef IOT_TOUCK_KEY_DEBUG
void iot_touch_key_io_dbg_bus_cfg(IOT_TK_IO_DBG dbg_bus_sel)
{
    touch_key_dbg_bus_sel_cfg(dbg_bus_sel);
}

void iot_touch_key_xphase_dbg_sig_cfg(IOT_TK_PHASE_ID phase_id, uint8_t dbg_sig_sel)
{
    touch_key_pmm_dbg_sig_sel_cfg((TK_PHASE_ID)phase_id, dbg_sig_sel);
}

void iot_touch_key_phase0_pad_id_cfg(IOT_TK_PAD_ID pad_id)
{
    touch_key_phase0_pad_id_cfg((TK_PAD_ID)pad_id);
    touch_key_set_pad_inuse((TK_PAD_ID)pad_id);
}

void iot_touch_key_phase0_aver_num_cfg(uint8_t aver_num)
{
    touch_key_phase0_aver_num_cfg(aver_num);
}

uint32_t iot_touch_key_read_phase_cdc(IOT_TK_PHASE_ID id)
{
    bool_t cdc_flag = false;
    uint32_t cdc_out;

    while (!cdc_flag) {
        cdc_flag = touch_key_phase_read_cdc_flag((TK_PHASE_ID)id);
    }

    cdc_out = touch_key_phase_read_cdc((TK_PHASE_ID)id);
    touch_key_phase_cdc_clear((TK_PHASE_ID)id);

    return cdc_out;
}

uint16_t iot_touch_key_read_cdc_raw(void)
{
    uint8_t cdc_flag = 0;
    uint16_t cdc_out;
    uint16_t dbg_bus_value;

    while (!cdc_flag) {
        dbg_bus_value = touch_key_read_tk_dbg_bus();
        cdc_flag = (dbg_bus_value >> 11) & 0x1;
    }

    cdc_out = 0x7ff & touch_key_read_tk_dbg_bus();

    while (cdc_flag) {
        dbg_bus_value = touch_key_read_tk_dbg_bus();
        cdc_flag = (dbg_bus_value >> 11) & 0x1;
    }

    return cdc_out;
}

uint8_t iot_touch_key_get_debug_info(IOT_TK_PAD_ID pad_id,
                                     iot_touch_key_debug_info_t *touch_debug_info)
{
    uint8_t ret;
    iot_touch_key_debug_info_t debug_info;

    ret = iot_touch_key_get_monitor_phase(pad_id, &debug_info.monior_phase);
    if (ret != RET_OK) {
        return ret;
    }

    ret = iot_touch_key_get_work_phase(pad_id, &debug_info.work_phase);
    if (ret != RET_OK) {
        return ret;
    }

    debug_info.cur_cdc = iot_touch_key_read_phase_nodelay_cdc(debug_info.monior_phase);
    debug_info.intr_cdc = iot_touch_key_info[debug_info.work_phase].cur_cdc;
    debug_info.diff_value = debug_info.cur_cdc - debug_info.intr_cdc;

    if (!debug_info.intr_cdc
        || debug_info.cur_cdc <= iot_touch_key_info[debug_info.work_phase].cur_cdc) {
        debug_info.pad_state = IOT_TOUCH_KEY_INT_PRESS_RELEASE;
    } else if (debug_info.cur_cdc > iot_touch_key_info[debug_info.work_phase].cur_cdc) {
        if (debug_info.diff_value < IOT_TOUCH_KEY_TOUCH_PHASE_TOLERAN) {
            debug_info.pad_state = IOT_TOUCH_KEY_INT_PRESS_RELEASE;
        } else {
            debug_info.pad_state = IOT_TOUCH_KEY_INT_PRESS_MID;
        }
    }

    debug_info.toleran = IOT_TOUCH_KEY_TOUCH_PHASE_TOLERAN;
    *touch_debug_info = debug_info;
    return RET_OK;
}

void iot_touch_key_dump_all_config(void)
{
    uint32_t tk_digpad_config = pmm_read_tk_digpad_cfg();
    uint32_t tk_clk_config = pmm_read_clk_cfg();
    uint32_t gpio09_config = pin_get_pin_config(PIN_72);
    uint32_t gpio10_config = pin_get_pin_config(PIN_73);
    uint32_t gpio11_config = pin_get_pin_config(PIN_74);
    uint32_t gpio12_config = pin_get_pin_config(PIN_75);

    for (uint32_t i = 0; i < 0x100; i = i + 0X10)   //dump 7 phase config info
    {
        DBGLOG_DRIVER_INFO("TK:0x%x,content:0x%x,0x%x,0x%x,0x%x\n",
                           (DTOP_ADI2TOUCH_KEY_REG_BASEADDR + i),
                           *((volatile uint32_t *)(DTOP_ADI2TOUCH_KEY_REG_BASEADDR + i)),
                           *((volatile uint32_t *)(DTOP_ADI2TOUCH_KEY_REG_BASEADDR + i + 4)),
                           *((volatile uint32_t *)(DTOP_ADI2TOUCH_KEY_REG_BASEADDR + i + 8)),
                           *((volatile uint32_t *)(DTOP_ADI2TOUCH_KEY_REG_BASEADDR + i + 0xC)));
    }

    DBGLOG_DRIVER_INFO(
        "TK tk_digpad_en:0x%x, tk_digpad_pd_rsv:0x%x, tk_digpad_pu_rsv:0x%x, clock config:0x%x\n",
        tk_digpad_config & 0x1F, (tk_digpad_config >> 6) & 0x3, (tk_digpad_config >> 8) & 0x3,
        tk_clk_config);

    DBGLOG_DRIVER_INFO(
        "TK tk0~3 pull-ip/down mapping to pmm_gpio09:0x%x, gpio10:0x%x, gpio11:0x%x, gpio12:0x%x\n",
        gpio09_config, gpio10_config, gpio11_config, gpio12_config);
}
#endif
