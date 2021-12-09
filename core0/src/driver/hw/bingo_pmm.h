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

#ifndef DRIVER_HW_BINGO_PMM_H
#define DRIVER_HW_BINGO_PMM_H

#include "hw_reg_api.h"
#include "adi_common.h"
#include "critical_sec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /* used for crystl_dac_clk_sel, default0:8MHz;1:16MHz */
    MADC_CRYSTL_DAC_CLK_SEL,
    /* LP bias trim enable */
    MADC_CTAL_CUR_BIAS,
} MADC_CTAL_CUR;

typedef enum {
    LDO_1P2_MODULE_MADC,
    LDO_1P2_MODULE_MIC_ADC,
    LDO_1P2_MODULE_DAC,
    LDO_1P2_MODULE_SYSPLL,
    LDO_1P2_MODULE_MAX
} LDO_1P2_MODULE_T;

#define BINGO_PMM_WR_REG_FIELD_(N, reg, ...)                     \
    ({                                                           \
        cpu_critical_enter();                                    \
        typeof(reg) _reg_x_;                                     \
        _reg_x_.w = bingo_pmm_read((uint32_t)&reg);              \
        CONCATENATE(WR_FIELD_, N)(_reg_x_, __VA_ARGS__);         \
        reg.w = _reg_x_.w;                                       \
        _reg_x_.w = reg.w; /* a dummy read to wait write valid*/ \
        cpu_critical_exit();                                     \
        UNUSED(_reg_x_);                                         \
    })

/**
 * WR_REG_FIELD(reg, field1, val1, field2, val2, field3, val3, ...)
 */
#define BINGO_PMM_WR_REG_FIELD(reg, ...) \
    BINGO_PMM_WR_REG_FIELD_(__REG_N_FIELD__(reg, __VA_ARGS__), reg, __VA_ARGS__)

#define BINGO_PMM_WR_REG(reg, value) \
    ({                               \
        reg.w = (value);             \
        uint32_t _temp = reg.w;      \
        UNUSED(_temp);               \
    })

#define BINGO_PMM_WR_REG_ONLY(reg, value) \
        ({                               \
            reg.w = (value);             \
        })

#define BINGO_PMM_RE_REG_FIELD(reg, bitfield)   \
    ({                                          \
        typeof(reg) __x;                        \
        cpu_critical_enter();                   \
        __x.w = bingo_pmm_read((uint32_t)&reg); \
        cpu_critical_exit();                    \
        (uint32_t) __x._b.bitfield;             \
    })

bool_t bingo_pmm_regaddr_is_in_range(uint32_t addr);
uint32_t bingo_pmm_read(uint32_t addr);
void bingo_pmm_pll_enable(bool_t enable);
void bingo_pmm_set_syspll_ndiv(uint8_t div);
uint32_t bingo_pmm_get_syspll_ndiv(void);

void bingo_pmm_adr_ctrl_cfg(uint32_t cfg);
void bingo_pmm_adc_ldo_enable(LDO_1P2_MODULE_T modue, bool_t enable);
void bingo_pmm_tia_ldo_enable(bool_t enable);
void bingo_pmm_pll_vco_tmp(uint8_t sel_atbana);
void bingo_pmm_crystl_ldo_bypass(bool_t en);
void bingo_pmm_set_crystl_current(uint8_t xtal_trim_code);
void bingo_pmm_crystl_current_config(void);
void bingo_pmm_sel_crystl_crrnt(uint8_t sel_atbana);
void bingo_pmm_sel1_crystl_cmlr(uint8_t sel_atbana);
void bingo_pmm_sel2_crystl_cmli(uint8_t sel_atbana);
void bingo_pmm_pll_vco_tmp_tune_code(uint8_t sel_atbana);
void bingo_pmm_set_lp_bias_trim(MADC_CTAL_CUR sel_atbana);
void bingo_pmm_clr_lp_bias_trim(MADC_CTAL_CUR sel_atbana);
void bingo_pmm_crystl_tn_rng_cap_arry_set(uint8_t val);
void bingo_pmm_l2n_crystl_tn_rng_cap_arry_set(uint8_t val);
void bingo_pmm_xtal_ldo_tune_code(uint32_t code);
void bingo_pmm_xtal_ldo_config(void);
void bingo_pmm_pst_mge0(uint8_t value);

void bingo_pmm_xtal_lowpower_config(bool_t enable);
void bingo_pmm_xtal_enable(bool_t enable);
void bingo_pmm_light_sleep_sel(uint8_t value);
void bingo_pmm_light_sleep_dtop_sel(uint8_t value);
void bingo_pmm_deep_sleep_config(uint8_t value);
void bingo_pmm_crystl_iso_en(bool_t enable);
void bingo_pmm_d_bg_pd_config(bool_t enable);
void bingo_pmm_en_ldo_crystl_config(bool_t enable);
void bingo_pmm_crystl_mux_reset(bool_t enable);
void bingo_pmm_shutdown_reset(void);
void bingo_pmm_bingo_adr_ctrl_wire(ADI_WIRE_MODE wire_mode);
void bingo_pmm_phy_dac_clock_sel(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_HW_BINGO_PMM_H */
