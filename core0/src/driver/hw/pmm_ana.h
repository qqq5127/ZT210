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

#ifndef _DRIVER_HW_PMM_ANA_H
#define _DRIVER_HW_PMM_ANA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PMM_ANA_CHG_CC,
    PMM_ANA_CHG_CV,
    PMM_ANA_CHG_UNKONWN,
} PMM_ANA_CHG_MODE;

typedef enum {
    PMM_ANA_DCDC0P8,
    PMM_ANA_DCDC1P2,
    PMM_ANA_DCDC1P8,
} PMM_ANA_DCDC;

PMM_ANA_CHG_MODE pmm_ana_get_charger_state(void);
void pmm_ana_set_charger_voltage(uint8_t vol);
void pmm_ana_charger_iout_res_cal(uint8_t res_code);
void pmm_ana_charger_iout_pmos_cal(uint8_t pmos_code);
void pmm_ana_atb_aon_sel(uint8_t sel_atbaon);
void pmm_ana_vtemp_sel(bool_t en);
void pmm_ana_dcdc_enable(bool_t en);
void pmm_ana_dcdc_zc_cfg(PMM_ANA_DCDC type, uint8_t val);
void pmm_ana_dcdc_on_cfg(PMM_ANA_DCDC type, uint8_t val);
void pmm_ana_dc0p8_max_eff_config(void);
void pmm_ana_dc1p2_max_eff_config(void);
void pmm_ana_dc1p8_max_eff_config(void);
void pmm_ana_vbat_detect_en(bool_t en);
void pmm_ana_vchg_detect_en(bool_t en);
void pmm_ana_charger_cur_mon_en(bool_t en);
bool_t pmm_ana_get_charger_cur_mon_flag(void);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_PMM_ANA_H */
