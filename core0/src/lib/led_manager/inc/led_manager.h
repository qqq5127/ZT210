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
#ifndef _LED_MANAGER_H
#define _LED_MANAGER_H

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup LED_MANAGER
 * @{
 * This section introduces the LIB LED_MANAGER module's enum, structure, functions and how to use this module.
 */

#include "types.h"
#include "gpio_id.h"

#ifdef __cplusplus
extern "C" {
#endif
#define LED_MAX_NUM 6


/** @defgroup lib_bt_mgmt_enum Enum
 * @{
 */

typedef enum {
    LED_MODE_BLINK,        /*blink mode*/
    LED_MODE_DIM,          /*breath mode*/
    LED_MODE_NORMAL_LIGHT, /*normal light mode*/
} led_mode_e;

/**
 * @}
 */

/** @defgroup lib_led_manager_struct Struct
 * @{
 */
typedef struct {
    led_mode_e mode;    /*led work mode, see led_mode_e*/
    bool_t high_on;     /*light when given high level or not*/
    uint32_t on_duty;   /*on duty*/
    uint32_t off_duty;  /*off duty*/
    uint32_t blink_cnt; /*on off counts in a loop*/
    uint32_t dim_duty;  /*breath duty*/
    uint32_t interval;  /*additional interval before next loop start*/
    uint32_t loop;      /*loop counts*/
} led_param_t;
/**
 * @}
 */
/**
 * @brief callback to handle led end
 *
 * @param cb led id
 */
typedef void (*led_end_callback)(uint8_t cb);

/**
 * @brief This function is to init led driver.
 *
 * @return uint32_t RET_OK for success else error.
 */
uint32_t led_init(void);

/**
 * @brief This function is to deinit led driver.
 *
 * @return uint32_t RET_OK for success else error.
 */
uint32_t led_deinit(void);

/**
 * @brief This function is to config led params.
 *
 * @param id is led number id.
 * @param led_param is led params.
 * @param cb is callback for notify app.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t led_config(RESOURCE_GPIO_ID id, const led_param_t *led_param, led_end_callback cb);

/**
 * @brief This function is to led start auto action follow config.
 *
 * @param id is led number id.
 * @param on is start with on or off.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t led_start_action(uint8_t id, bool_t on);

/**
 * @brief This function is to make led off.
 *
 * @param id is led number id.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t led_off(uint8_t id);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup LED_MANAGER
 */

/**
 * @}
 * addtogroup LIB
 */

#endif   //_LED_MANAGER_H
