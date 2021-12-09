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
#include "bingo_pmm.h"
#include "bingo_ana.h"

#include "iot_timer.h"
#include "caldata.h"
#include "iot_spk_dac.h"

/*@function: DAC VREF test
@port: select left channel or right channel
*/
void iot_spk_dac_vref_trim(uint8_t port, IOT_SPK_DAC_RDAC_VREF vref_out)
{
    bingo_ana_rdac_vref_ctrl(port, (BINGO_ANA_RDAC_VREF)vref_out);
}

/*@function: DAC/SPK VCM test
@port: select left channel or right channel
*/
void iot_spk_dac_vcm_trim(uint8_t port, uint8_t vcm_out)
{
    bingo_ana_rdac_vcm_ctrl(port, vcm_out);
}

/*@function: avoid pop sound, for speaker dac channel enable
@port: select left channel or right channel
*/
void iot_spk_dac_enable(uint8_t port)
{
    /*codec ldo1p2 enable*/
    bingo_pmm_adc_ldo_enable(LDO_1P2_MODULE_DAC, true);
    /*
    RDAC/RDAC_DATA left enable
    RDAC right reference fast start up
    */
    bingo_ana_rdac_data_en(port, true);
    bingo_ana_rdac_en(port, true);
    bingo_ana_rdac_sst_n(port, true);
    iot_timer_delay_us(200);
    bingo_ana_rdac_sst_n(port, false);
    iot_timer_delay_us(20);
}

void iot_spk_dac_start(uint8_t port)
{
    bingo_ana_driver_mute_enable(port, true);
    iot_timer_delay_us(10);
    /*driver configure*/
    bingo_ana_1st_stage_enable(port, true);
    iot_timer_delay_us(10);
    bingo_ana_2nd_stage_enable(port, true);
    iot_timer_delay_us(10);
    bingo_ana_3rd_stage_phase1_enable(port, true);
    iot_timer_delay_us(10);
    bingo_ana_3rd_stage_phase2_enable(port, true);
    iot_timer_delay_us(10);
    bingo_ana_3rd_stage_phase3_enable(port, true);
    iot_timer_delay_us(50);

    /*left driver output release*/
    bingo_ana_driver_mute_enable(port, false);
    iot_timer_delay_us(10);
}

/*@function: to avoid pop sound, for speaker dac channel disable
@port: select left channel or right channel
*/
void iot_spk_dac_disable(uint8_t port)
{
    bingo_ana_driver_mute_enable(port, true);
    iot_timer_delay_us(10);

    bingo_ana_3rd_stage_phase3_enable(port, false);
    iot_timer_delay_us(10);
    bingo_ana_3rd_stage_phase2_enable(port, false);
    iot_timer_delay_us(10);
    bingo_ana_3rd_stage_phase1_enable(port, false);
    iot_timer_delay_us(10);
    bingo_ana_2nd_stage_enable(port, false);
    iot_timer_delay_us(10);
    bingo_ana_1st_stage_enable(port, false);
    iot_timer_delay_us(10);

    bingo_ana_rdac_data_en(port, false);
    bingo_ana_rdac_en(port, false);
    iot_timer_delay_us(10);

    bingo_ana_driver_mute_enable(port, false);
    iot_timer_delay_us(50);
}

/*@function: speaker compensation trimming
@port: select left channel or right channel
@is_pos: select positive or negative channel
@com_pos: compensation value
*/
void iot_spk_dac_dc_offset_trim(uint8_t port, bool_t is_pos, uint16_t comp_pos)
{
    bingo_ana_driver_os_comp_trim(port, is_pos, comp_pos);
}

/*@function: Speaker initialization
@port: select left channel or right channel
*/
void iot_spk_dac_init(uint8_t port)
{
    uint8_t vcm_trim_code  = 0;
    uint8_t vref_trim_code  = 0;
    uint16_t spk_coden  = 0;
    uint16_t spk_codep  = 0;
    if(port == IOT_SPK_DAC_PORT_0){
        vcm_trim_code = (uint8_t)cal_data_trim_code_get(LSPK_VCM_TRIM_CODE_0P7);
        vref_trim_code = (uint8_t)cal_data_trim_code_get(LSPK_VREF_TRIM_CODE);
        spk_coden = (uint16_t)((cal_data_trim_code_get(LSPK_DC_TRIM_CODE)>>8) & 0xff);
        spk_codep = (uint16_t)(cal_data_trim_code_get(LSPK_DC_TRIM_CODE) & 0xff);
    } else {
        vcm_trim_code = (uint8_t)cal_data_trim_code_get(RSPK_VCM_TRIM_CODE_0P7);
        vref_trim_code = (uint8_t)cal_data_trim_code_get(RSPK_VREF_TRIM_CODE);
        spk_coden = (cal_data_trim_code_get(RSPK_DC_TRIM_CODE)>>8) & 0xff;
        spk_codep = cal_data_trim_code_get(RSPK_DC_TRIM_CODE) & 0xff;
    }

    vref_trim_code += IOT_VREF_1P2_1P4_DELTA_CODE;
    iot_spk_dac_vref_trim(port, (IOT_SPK_DAC_RDAC_VREF)vref_trim_code);
    iot_spk_dac_vcm_trim(port, vcm_trim_code);
    iot_spk_dac_dc_offset_trim(port, true, spk_codep);
    iot_spk_dac_dc_offset_trim(port, false, spk_coden);
}

/*@function: Speaker gain control
@port: select left channel or right channel
*/
void iot_spk_dac_gain(uint8_t port, uint8_t gain);

/*@function: Speaker deinitialize
@port: select left channel or right channel
*/
void iot_spk_dac_deinit(void)
{
    /*codec ldo1p2 disable*/
    bingo_pmm_adc_ldo_enable(LDO_1P2_MODULE_DAC, false);
}
