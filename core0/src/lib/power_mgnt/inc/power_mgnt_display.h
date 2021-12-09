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

#ifndef LIB_POWER_MGNT_DISPLAY_H
#define LIB_POWER_MGNT_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 * @brief This function is to display power mgnt performance.
 *
 * @param cpu is the cpu.
 * @param interval is the interval between display.
 */
void power_mgnt_performance_display(uint32_t cpu, uint32_t interval);

#ifdef __cplusplus
}
#endif

#endif /* LIB_POWER_MGNT_DISPLAY_H */
