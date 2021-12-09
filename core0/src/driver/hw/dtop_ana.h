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
#ifndef _DRIVER_HW_DTOP_ANA_H_
#define _DRIVER_HW_DTOP_ANA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pin.h"

uint8_t dtop_ana_claim_as_gpio(uint8_t adc_pin);
uint8_t dtop_ana_claim_as_adc(uint8_t adc_pin);
uint8_t dtop_ana_release_pin(uint8_t adc_pin);
uint8_t dtop_ana_set_pull_mode(uint8_t adc_pin, PIN_PULL_MODE mode);
uint8_t dtop_ana_set_drv_strength(uint8_t adc_pin, uint8_t drv);
void dtop_ana_gpio_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_DTOP_ANA_H_ */
