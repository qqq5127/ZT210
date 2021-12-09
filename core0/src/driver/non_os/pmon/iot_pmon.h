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
#ifndef _DRIVER_NON_OS_PMON_RING_H_
#define _DRIVER_NON_OS_PMON_RING_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is to calculate the average count of four rings over a specified period
 *
 * @return uint32_t  A return value between 0x62 and 0xb9 is the normal range for ring oscillator in the specified cycle.
 */
uint32_t iot_pmon_get_score(void);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_NON_OS_PMON_RING_H_*/
