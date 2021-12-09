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

#ifndef _EQUALISER_H
#define _EQUALISER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void eq_set_en(bool_t en);
void eq_set_coef_en(bool_t coef_en);
void eq_set_srst(bool_t en);
void eq_set_start(bool_t en);
void eq_set_done(bool_t en);
uint32_t eq_get_done(void);

/**
 * @brief eq_enable() - eq clk and en open
 */
void eq_enable(void);
void eq_disable(void);

/**
 * @brief eq_set_mode() - eq mode configuration
 * @param len           the data number of each iir filter in and out
 * @param start_band    the start band position of eq_iir_coef
 * @param num           the eq iir filter numbers for each eq calculation
 * @param data_mode     0:32 bit mode, 1:16 bit mode
 * @param flt_mode      1'h1:input is float mode, 0:input is integer mode
 */
void eq_set_mode( uint8_t start_band, uint8_t num,
                 uint8_t data_mode, uint8_t flt_mode, uint16_t len);
void eq_set_data_mode(uint8_t data_mode);
void eq_set_load_mem_mode(uint8_t mode);
void eq_set_ahb_bytes(uint8_t bytes);
void eq_set_shift(uint8_t in_lsh_bit_sel, uint8_t out_rsh_bit_sel);
void eq_set_eq_flt(uint8_t flt_rnd_sel, uint8_t ignore_flt2i_st,
                   uint8_t flt2i_st, uint8_t i2flt_st);
void eq_set_exp_thre(uint8_t exp_thre);
void eq_set_data_rd_addr(uint8_t *rd_addr);
void eq_set_data_wr_addr(uint8_t *wr_addr);
void eq_set_coef_rd_addr(uint8_t *rd_addr);
void eq_set_coef_wr_addr(uint8_t *wr_addr);
void eq_set_coef_cmd(uint8_t coef_done, uint8_t coef_start, uint8_t coef_rw,
                     uint8_t coef_en);
uint32_t eq_get_coef_done(void);
void eq_set_it_enable(uint8_t data_done_en, uint8_t coef_done_en);
uint32_t eq_get_it_coef_done(void);
uint32_t eq_get_it_data_done(void);
uint32_t eq_get_it_data_done_raw(void);
uint32_t eq_get_it_coeff_done_raw(void);
void eq_set_it_coef_done_clr(uint8_t en);
void eq_set_it_data_done_clr(uint8_t en);


#ifdef __cplusplus
}
#endif

#endif /* _EQUALISER_H */
