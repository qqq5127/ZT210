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
#ifndef _DRIVER_HW_GPIO_MTX_H
#define _DRIVER_HW_GPIO_MTX_H

#include "gpio_mtx_signal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_MTX_SIGNAL_GPIO GPIO_MTX_OUT_DEFAULT

typedef enum {
    GPIO_MTX_MODE_CORE,
    GPIO_MTX_MODE_MATRIX,
    GPIO_MTX_MODE_MAX,
} GPIO_MTX_MODE;

void gpio_mtx_set_in_signal(uint16_t pin, GPIO_MTX_SIGNAL_IN in, GPIO_MTX_MODE mode);
void gpio_mtx_set_out_signal(uint16_t pin, GPIO_MTX_SIGNAL_OUT out);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_GPIO_MTX_H */
