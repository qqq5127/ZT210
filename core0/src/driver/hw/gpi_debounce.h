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

#ifndef DRIVER_HW_GPI_DEBOUNCE_H
#define DRIVER_HW_GPI_DEBOUNCE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GPI_DEB_DIG,
    GPI_DEB_PMM,
    GPI_DEB_MAX,
} GPI_DEB;

typedef enum {
    GPI_DEB_CH_0,
    GPI_DEB_CH_1,
    GPI_DEB_CH_2,
    GPI_DEB_CH_3,
    GPI_DEB_CH_MAX,
} GPI_DEB_CH;

typedef enum {
    GPI_DEB_INT_IO,
    GPI_DEB_INT_PRESS,
    GPI_DEB_INT_PRESS_MID,
    GPI_DEB_INT_MAX,
} GPI_DEB_INT;

void gpi_debounce_init(GPI_DEB id);
void gpi_debounce_deinit(GPI_DEB id);
void gpi_debounce_enable(GPI_DEB id, GPI_DEB_CH ch);
void gpi_debounce_disable(GPI_DEB id, GPI_DEB_CH ch);
void gpi_debounce_force_input_low(GPI_DEB id, GPI_DEB_CH ch, bool_t enable);
void gpi_debounce_force_input_high(GPI_DEB id, GPI_DEB_CH ch, bool_t enable);
void gpi_debounce_level_sel(GPI_DEB id, GPI_DEB_CH ch, uint8_t level);
void gpi_debounce_out_edge_sel(GPI_DEB id, GPI_DEB_CH ch, uint8_t sel);
void gpi_debounce_set_in_polarity(GPI_DEB id, GPI_DEB_CH ch, bool_t enable);
void gpi_debounce_set_out_polarity(GPI_DEB id, GPI_DEB_CH ch, bool_t enable);
void gpi_debounce_src_select(GPI_DEB id, GPI_DEB_CH ch, uint8_t src);
void gpi_debounce_set_common_clock_div(GPI_DEB id, uint8_t div);
void gpi_debounce_set_clock_div(GPI_DEB id, GPI_DEB_CH ch, uint16_t div);
uint8_t gpi_debounce_get_int_state(GPI_DEB id, GPI_DEB_CH ch);
uint32_t gpi_debounce_get_int_state_all(GPI_DEB id);
void gpi_debounce_int_enable(GPI_DEB id, GPI_DEB_CH ch, GPI_DEB_INT int_type);
void gpi_debounce_int_disable(GPI_DEB id, GPI_DEB_CH ch, GPI_DEB_INT int_type);
void gpi_debounce_int_clear(GPI_DEB id, GPI_DEB_CH ch, GPI_DEB_INT int_type);
void gpi_debounce_int_clear_all(GPI_DEB id, GPI_DEB_CH ch);
void gpi_debounce_set_threshold(GPI_DEB id, GPI_DEB_CH ch, uint8_t thd);
void gpi_debounce_reset_enable(GPI_DEB id, GPI_DEB_CH ch, bool_t enable);
uint8_t gpi_debounce_get_reset_flag(GPI_DEB id, GPI_DEB_CH ch);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_HW_GPI_DEBOUNCE_H */
