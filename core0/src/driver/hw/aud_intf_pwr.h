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
#ifndef _DRIVER_HW_AUD_INTF_PWR_H
#define _DRIVER_HW_AUD_INTF_PWR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void aud_intf_pwr_on(uint8_t aud_intf);
void aud_intf_pwr_off(uint8_t aud_intf);
uint32_t aud_intf_pwr_get_mask(void);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_AUD_INTF_PWR_H */
