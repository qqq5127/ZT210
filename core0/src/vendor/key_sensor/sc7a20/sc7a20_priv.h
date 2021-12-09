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

#ifndef _SC7A20_PRIV_H_
#define _SC7A20_PRIV_H_

#include "gpio_id.h"

/**reg map***************/
#define SC7A20_OUT_TEMP_L     0x0C
#define SC7A20_OUT_TEMP_H     0x0D
#define SC7A20_WHO_AM_I       0x0F
#define SC7A20_USER_CAL_START 0x13
#define SC7A20_USER_CAL_END   0x1A
#define SC7A20_NVM_WR         0x1E
#define SC7A20_TEMP_CFG       0x1F
#define SC7A20_CTRL_REG1      0x20
#define SC7A20_CTRL_REG2      0x21
#define SC7A20_CTRL_REG3      0x22
#define SC7A20_CTRL_REG4      0x23
#define SC7A20_CTRL_REG5      0x24
#define SC7A20_CTRL_REG6      0x25
#define SC7A20_REFERENCE      0x26
#define SC7A20_STATUS_REG     0x27
#define SC7A20_OUT_X_L        0x28
#define SC7A20_OUT_X_H        0x29
#define SC7A20_OUT_Y_L        0x2A
#define SC7A20_OUT_Y_H        0x2B
#define SC7A20_OUT_Z_L        0x2C
#define SC7A20_OUT_Z_H        0x2D
#define SC7A20_FIFO_CTRL_REG  0x2E
#define SC7A20_SRC_REG        0x2F
#define SC7A20_AOI1_CFG       0x30
#define SC7A20_AOI1_SOURCE    0x31
#define SC7A20_AOI1_THS       0x32
#define SC7A20_AOI1_DURATION  0x33
#define SC7A20_AOI2_CFG       0x34
#define SC7A20_AOI2_SOURCE    0x35
#define SC7A20_AOI2_THS       0x36
#define SC7A20_AOI2_DURATION  0x37
#define SC7A20_CLICK_CFG      0x38
#define SC7A20_CLICK_SRC      0x39
#define SC7A20_CLICK_THS      0x3A
#define SC7A20_TIME_LIMIT     0x3B
#define SC7A20_TIME_LATENCY   0x3C
#define SC7A20_TIME_WINDOW    0x3D
#define SC7A20_ACT_THS        0x3E
#define SC7A20_ACT_DURATION   0x3F

#define BASIC_MID_BASE         0
#define SC7A20_INT_PIN_DEFAULT 53

/**SC7A20 SDO to GND          0x18 */
/**SC7A20 SDO to VCC          0x19 */
#define SC7A20_ADDR (0x19U)

/**
 * Gsensor INT PIN,
 * 1 means INT1
 * 2 means INT2
 */
#define GSENSOR_SC7A20_INT_PIN 1
#define SC7A20_INT_GPIO        GPIO_CUSTOMIZE_5

/**Gsensor SCL/SDA pin*/
#define GSENSOR_SC7A20_INT_SCL 00
#define GSENSOR_SC7A20_INT_SDA 03

#define GSENSOR_ENABLE_TRIPLE_CLICK 0

/**
 * Gsensor Driver Release Enable
 * 1 means enable (log disable),
 * 0 means disable (log enable)
 */
#define SL_SENSOR_ALOG_RELEASE_ENABLE 1

#endif /* _SC7A20_PRIV_H_ */
