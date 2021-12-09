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
#ifndef _DRIVER_HW_PMON_H
#define _DRIVER_HW_PMON_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PMON_RING_ID_0,
    PMON_RING_ID_1,
    PMON_RING_ID_2,
    PMON_RING_ID_3,
    PMON_RING_ID_MAX,
} PMON_RING_ID;

#define PMON_CNT_CYCLE 16

void pmon_enable(void);
void pmon_disable(void);
uint32_t pmon_get_count(uint32_t ring_id);
uint32_t pmon_get_aver_count(void);

#endif /* DRIVER_HW_PMON_H */
