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
#ifndef _DRIVER_HAL_LEDC_H
#define _DRIVER_HAL_LEDC_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup LEDC
 * @{
 * This section introduces the LEDC module's enum, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PMM_LEDC_MAX_NUM  2
#define DTOP_LEDC_MAX_NUM 4

#define LEDC_DIM_MIN_MS        500
#define LEDC_ON_OFF_MIN_MS     10
#define LEDC_TMR_COMPENSATE_MS 100

typedef enum {
    IOT_LED_LEDC_MODULE_PMM,
    IOT_LED_LEDC_MODULE_DTOP,
    IOT_LED_LEDC_MODULE_MAX_NUM,
} IOT_LED_LEDC_MODULE;

enum {
    LEDC_MODE_NORMAL_LIGHT,
    LEDC_MODE_BLINK,
    LEDC_MODE_BREATH,
    LEDC_MODE_MAX,
};

/** @brief LED breath. */
typedef enum {
    IOT_LED_ACTION_MODE_OFF2ON,
    IOT_LED_ACTION_MODE_ON2OFF,
} IOT_LED_ACTION_MODE;

/**
 * @brief This function is to select ledc pin.
 *
 * @param module is decide to use pmm or dtop ledc
 * @param ledc_id is led number id.
 * @param pin is led io pin.
 * @return int32_t RET_OK for success else RET_FAIL.
 */
int32_t iot_ledc_pin_sel(IOT_LED_LEDC_MODULE module, uint8_t ledc_id, uint16_t pin);

/**
 * @brief This function is to release ledc pin.
 *
 * @param module is decide to use pmm or dtop ledc
 * @param pin is led io pin.
 */
void iot_ledc_pin_release(IOT_LED_LEDC_MODULE module, uint16_t pin);

/**
 * @brief This function is to init ledc ctrl clk&output mapping.
 *
 * @param module is decide to use pmm or dtop ledc
 */
void iot_ledc_init(IOT_LED_LEDC_MODULE module);

/**
 * @brief This function is to assign a sub ledc
 *
 * @param module is decide to use pmm or dtop ledc
 * @return int8_t sub ledc id or -1 which means fail.
 */
int8_t iot_ledc_assign(IOT_LED_LEDC_MODULE module);

/**
 * @brief This function is to open a ledc&tmr set.
 *
 * @param module is decide to use pmm or dtop ledc
 * @param id is led number id.
 * @return uint8_t RET_OK for success else RET_FAIL.
 */
uint8_t iot_ledc_open(IOT_LED_LEDC_MODULE module, uint8_t id);

/**
 * @brief This function is to set ledc on.
 *
 * @param module is decide to use pmm or dtop ledc
 * @param id is led number id.
 * @return uint8_t RET_OK for success else RET_FAIL.
 */
uint8_t iot_ledc_on(IOT_LED_LEDC_MODULE module, uint8_t id);

/**
 * @brief This function is to config ledc breath mode
 *
 * @param module is decide to use pmm or dtop ledc
 * @param ledc_id is ledc number id.
 * @param dim is dim period ms.
 * @param high_on give high or low level to light led.
 * @return uint32_t RET_OK for success else RET_FAIL.
 */
uint32_t iot_ledc_breath_config(IOT_LED_LEDC_MODULE module, uint8_t ledc_id, uint32_t dim,
                                bool_t high_on);

/**
 * @brief This function is to config ledc blink mode.
 *
 * @param module Decide to use pmm or dtop ledc
 * @param ledc_id Ledc number id.
 * @param on_duty Dim period ms.
 * @param off_duty Dim period ms.
 * @param high_on give high or low level to light led.
 * @param mode Action mode.
 * @return uint32_t RET_OK for success else RET_FAIL.
 */
uint32_t iot_ledc_blink_config(IOT_LED_LEDC_MODULE module, uint8_t ledc_id, uint32_t on_duty,
                               uint32_t off_duty, bool_t high_on, IOT_LED_ACTION_MODE mode);

/**
 * @brief This function is to config ledc normal light.
 *
 * @param module is decide to use pmm or dtop ledc
 * @param ledc_id is ledc number id.
 * @param high_on give high or low level to light led.
 * @return uint32_t RET_OK for success else RET_FAIL.
 */
uint32_t iot_ledc_normal_light_config(IOT_LED_LEDC_MODULE module, uint8_t ledc_id, bool_t high_on);

/**
 * @brief This function is to set ledc off.
 *
 * @param module is decide to use pmm or dtop ledc
 * @param id is ledc number id.
 * @return uint8_t RET_OK for success else RET_FAIL.
 */
uint8_t iot_ledc_off(IOT_LED_LEDC_MODULE module, uint8_t id);

/**
 * @brief This function is to close a ledc&tmr set.
 *
 * @param module is decide to use pmm or dtop ledc
 * @param id is ledc number id.
 * @return uint8_t RET_OK for success else RET_FAIL.
 */
uint8_t iot_ledc_close(IOT_LED_LEDC_MODULE module, uint8_t id);

/**
 * @brief This function is to deinit ledc ctrl clk&output mapping.
 */
void iot_ledc_deinit(void);

/**
 * @brief This function is to make ledc breath from on to off and continue off.
 *
 * @param module is decide to use pmm or dtop ledc
 * @param id is ledc number id.
 * @param high_on give high or low level to light led.
 */
void iot_ledc_breath_on2off(IOT_LED_LEDC_MODULE module, uint8_t id, bool_t high_on);

/**
 * @brief This function is to make ledc breath from off to on and continue on.
 *
 * @param module is decide to use pmm or dtop ledc
 * @param id is ledc number id.
 * @param high_on give high or low level to light led.
 */
void iot_ledc_breath_off2on(IOT_LED_LEDC_MODULE module, uint8_t id, bool_t high_on);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup LEDC
 * @}
 * addtogroup HAL
 */
#endif   //_DRIVER_HAL_LEDC_H
