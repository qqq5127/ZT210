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

#ifndef DRIVER_HW_PLL_CAL_H
#define DRIVER_HW_PLL_CAL_H

#ifdef __cplusplus
extern "C" {
#endif

void pll_cal_syspll_calibration(uint8_t ndiv);
void pll_sleep_auto_wake_en(bool_t enable);

uint32_t pll_cal_reg_read(uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_HW_PLL_CAL_H */
