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
#include "anc.h"
#include "riscv_cpu.h"
#include "aud_intf_pwr.h"

/* hal includes */
#include "iot_irq.h"
#include "iot_anc.h"

#define IOT_ANC_COEFF_PARAM_NUM         160
#define IOT_ANC_SAMPLE_RATE_FRAME_CNT   1
#define IOT_ANC_CLIPPING_RATE           4
#define IOT_ANC_CLIPPING_DB_MAX         7
/* 0 ~ -6dBfs clipping limit: 0.50, 0.45, 0.40, 0.35, 0.32, 0.28, 0.25, * 2^23 */
static const uint32_t iot_anc_clipping_threshold[IOT_ANC_CLIPPING_DB_MAX] = {
     3856469, 3774875, 3355443, 2936013, 2684355, 2348810, 2097152,
};


static iot_anc_switch_mode_completed_cb anc_switch_mode_done_cb;
static uint32_t iot_anc_isr_handler(uint32_t vector, uint32_t data);

void iot_anc_enable_sync(bool_t sync)
{
    if(sync)  {
        audio_cfg_multipath(AUDIO_MODULE_ANC);
    } else {
        audio_enable_module(AUDIO_MODULE_ANC);
    }
}

uint8_t iot_get_anc_clk_enable(void)
{
    uint32_t clk_eb = audio_get_global_clk_status();

    return (clk_eb & BIT(AUDIO_MODULE_ANC)) ? 1 : 0;
}

void iot_anc_disable_sync(bool_t sync)
{
    if(sync) {
        audio_release_multipath(AUDIO_MODULE_ANC);
    } else {
        audio_disable_module(AUDIO_MODULE_ANC);
    }
}

void iot_anc_multipath_sync(void)
{
    audio_multipath_enable();
}

void iot_anc_start(void)
{
    anc_enable(true);
}

void iot_anc_stop(void)
{
    anc_enable(false);
    //audio intf power off vote
    aud_intf_pwr_off(AUDIO_MODULE_ANC);
}

void iot_anc_src_select(IOT_ANC_INPUT_SRC_ID id, IOT_ANC_RX_SRC_VALUE value)
{
    if ((id == IOT_ANC_INPUT_SRC_ASRC_0) || (id == IOT_ANC_INPUT_SRC_ASRC_1)) {
        audio_set_anc_src((AUDIO_ANC_INPUT_ID)id, (AUDIO_ANC_RX_SRC_ID)value);
        anc_out_bypass_asrc(ANC_OUT_DFE_0, true);
        anc_out_bypass_asrc(ANC_OUT_DFE_1, true);
    } else {
        audio_set_anc_src((AUDIO_ANC_INPUT_ID)id, (AUDIO_ANC_RX_SRC_ID)value);
    }
}

void iot_anc_in_component_link(IOT_ANC_COMPONENT_ID id, IOT_ANC_IN_SRC_ID in)
{
    anc_in_link((ANC_COMPONENT_ID)id, (ANC_IN_SRC_ID)in);
}

void iot_anc_out_component_link(IOT_ANC_COMPONENT_ID id, IOT_ANC_OUT_ID out)
{
    anc_out_link((ANC_COMPONENT_ID)id, (ANC_OUT_ID)out);
}

void iot_anc_out_component_delink(IOT_ANC_COMPONENT_ID id, IOT_ANC_OUT_ID out)
{
    anc_out_link_delect((ANC_COMPONENT_ID)id, (ANC_OUT_ID)out);
}

void iot_anc_drc_set(int full_limit, bool_t enable)
{
    uint32_t threshold;

    full_limit = -full_limit;//to get array idx

    if (enable) {
        full_limit = full_limit >= IOT_ANC_CLIPPING_DB_MAX ?
                    IOT_ANC_CLIPPING_DB_MAX - 1 : full_limit;
    }

    assert(full_limit >= 0 && full_limit < IOT_ANC_CLIPPING_DB_MAX);

    threshold = iot_anc_clipping_threshold[full_limit];

    anc_out_clipping_thres(ANC_OUT_DFE_1, threshold);
    anc_out_clipping_rate(ANC_OUT_DFE_1, IOT_ANC_CLIPPING_RATE);
}

void iot_anc_config(uint32_t rate, const uint32_t *anc_coeff0, const uint32_t *anc_coeff1)
{
    uint8_t clk_en;
    uint32_t mask = cpu_disable_irq();

    clk_en = iot_get_anc_clk_enable();

    //audio intf power on vote
    aud_intf_pwr_on(AUDIO_MODULE_ANC);

    anc_sample_rate(rate, IOT_ANC_SAMPLE_RATE_FRAME_CNT);

    if (!clk_en) {
        iot_anc_enable_sync(false);
    }

    anc_sw_coeff_access_enable(true);

    anc_write_coeff_param(ANC_COEFF_PARAM_0, 0, anc_coeff0, IOT_ANC_COEFF_PARAM_NUM);
    anc_write_coeff_param(ANC_COEFF_PARAM_1, 0, anc_coeff1, IOT_ANC_COEFF_PARAM_NUM);
    anc_sw_coeff_access_enable(false);

    if (!clk_en) {
        iot_anc_disable_sync(false);
    }

    cpu_restore_irq(mask);
}

void iot_anc_interrupt_open(void)
{
    iot_irq_t anc_isr;

    anc_isr = iot_irq_create(ANC_INT, 0, iot_anc_isr_handler);
    anc_it_clear(true);
    iot_irq_unmask(anc_isr);
    anc_it_enable(true);
}

void iot_anc_frame_division_counter(uint8_t frame_div_cnt)
{
    anc_frame_div_counter(frame_div_cnt);
}

uint8_t iot_anc_write_coeff_param(IOT_ANC_COEFF_PARAM_SET set, uint32_t offset, const uint32_t *param,
                                  uint32_t len)
{
    return anc_write_coeff_param((ANC_COEFF_PARAM_SET)set, offset, param, len);
}

void iot_anc_write_coeff_table(const uint32_t *anc_coeff, iot_anc_switch_mode_completed_cb cb)
{
    ANC_COEFF_PARAM_SET set;

    anc_sw_coeff_access_enable(true);
    set = (ANC_COEFF_PARAM_SET)(anc_get_coeff_select() ^ 1);
    anc_write_coeff_param(set, 0, anc_coeff, IOT_ANC_COEFF_PARAM_NUM);
    anc_sw_coeff_access_enable(false);

    anc_sw_coeff_select(set);
    if(cb != NULL) {
            anc_switch_mode_done_cb = cb;
    }
}

static uint32_t iot_anc_isr_handler(uint32_t vector, uint32_t data) IRAM_TEXT(iot_anc_isr_handler);
static uint32_t iot_anc_isr_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);
    anc_it_enable(false);
    if(anc_switch_mode_done_cb != NULL) {
        anc_switch_mode_done_cb();
    }
    anc_it_enable(true);
    anc_it_clear(true);

    return RET_OK;
}

bool_t iot_anc_get_switch_mode_done(void)
{
    return anc_get_it_raw_status();
}

void iot_anc_switch_mode_done_clear(void)
{
    anc_it_clear(true);
}
