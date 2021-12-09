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

#ifndef __DRIVER_HW_ADI_SLAVE_H__
#define __DRIVER_HW_ADI_SLAVE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Register Configuration of sel_atb_rsv */
typedef enum {
    MADC_BT_ADC_LDO,
    MADC_PHO_TIA_LDO,
    MADC_SEL_ATB_RSV_MAX,
} MADC_SEL_ATB_RCV;

void adi_slave_loop_mode_enable(bool_t enable);
void adi_slave_sync(void);
void adi_slave_adc_mic_cnt1_max(uint8_t cnt1);
void adi_slave_adc_mic_ratio(uint8_t ratio);
void adi_slave_set_bt_afe_rsv(MADC_SEL_ATB_RCV sel_atbana);
void adi_slave_clr_bt_afe_rsv(MADC_SEL_ATB_RCV sel_atbana);
void adi_slave_clr_all_bt_afe_rsv(void);
uint16_t adi_slave_dbg_bus(void);
uint32_t adi_slave_mic_rc_cap(void);
void adi_slave_set_adie_pll_divider(uint8_t div);
uint8_t adi_slave_get_adie_pll_divider(void);
void adi_slave_bist_set_vector(const uint32_t *vector);
uint32_t adi_slave_bist_get_error_count(void);
void adi_slave_meter_mic_control(uint32_t freq);
void adi_slave_meter_chn_start(uint8_t number);
uint32_t adi_slave_reg_read(uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif
