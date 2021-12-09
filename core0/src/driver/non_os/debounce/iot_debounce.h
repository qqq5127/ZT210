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

#ifndef _DRIVER_NON_OS_DEBOUNCE_H
#define _DRIVER_NON_OS_DEBOUNCE_H
/**
 * @addtogroup HAL
 * @{
 * @addtogroup DEBOUNCE
 * @{
 * This section introduces the DEBOUNCE module's enum, structure, functions and how to use this driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

#define IOT_DEBOUNCE_CHARGER_RESET_TIME   250

/** @defgroup hal_debounce_enum Enum
 * @{
 */

/**
 * @brief DEBOUNCE INT.TYPE
 * button      press                    release
 *        ______|                         |________________
 *              |_______________..._______|
 *              |-20ms-|                  |
 * time         0     20ms              n-S
 * int type    IO    PRESS_MID          PRESS
 */
typedef enum {
    IOT_DEBOUNCE_INT_IO,
    IOT_DEBOUNCE_INT_PRESS,
    IOT_DEBOUNCE_INT_PRESS_MID,
    IOT_DEBOUNCE_INT_MAX,
} IOT_DEBOUNCE_INT;

/** @brief DEBOUNCE INT.*/
typedef enum {
    IOT_DEBOUNCE_EDGE_RAISING,
    IOT_DEBOUNCE_EDGE_FALLING,
    IOT_DEBOUNCE_MODE_MAX
} IOT_DEBOUNCE_MODE;

typedef enum {
    IOT_DEBOUNCE_NO_TIMER,
    IOT_DEBOUNCE_ONE_SECOND,
    IOT_DEBOUNCE_TWO_SECONDS
}IOT_DEBOUNCE_CLEAR_TIMER_MODE;
/**
 * @}
 */

/** @defgroup hal_debounce_typedef Typedef
  * @{
 */
typedef void (*iot_debounce_callback)(uint16_t gpio, IOT_DEBOUNCE_INT int_type);
/**
 * @}
 */

/** @defgroup hal_debounce_struct Struct
 * @{
 */
typedef struct {
    iot_debounce_callback cb;
    bool_t int_io_en;
    bool_t int_press_en;
    bool_t int_press_mid_en;
} iot_debounce_int_cfg_t;
/**
 * @}
 */

/**
 * @brief This function is to init gpio debounce.
 */
void iot_debounce_init(void);

/**
 * @brief This function is to deinit gpio debounce.
 */
void iot_debounce_deinit(void);

/**
 * @brief This function is to set main driver.
 *
 * @param div is main driver
 */
void iot_debounce_set_main_divider(uint8_t div);

/**
 * @brief This function is to set gpio's debounce interrupt.
 *
 * @param ch is channel
 * @param cfg is gpio debounce's config
 */
void iot_debounce_set_interrupt(uint8_t ch, const iot_debounce_int_cfg_t *cfg);

/**
 * @brief This function is to config gpio's debounce.
 *
 * @param gpio is gpio num
 * @param time_ms is threshold of gpio's debounce
 * @param mode is gpio mode
 * @param cfg is gpio config
 * @return uint8_t selected gpio value.
 */
uint8_t iot_debounce_gpio(uint16_t gpio, uint8_t time_ms, IOT_DEBOUNCE_MODE mode,
                          const iot_debounce_int_cfg_t *cfg);

/**
 * @brief This function is to disable gpio's debounce.
 *
 * @param gpio GPIO num
 * @return RET_OK if success otherwise failed @see RET_TYPE
 */
uint8_t iot_debounce_gpio_disable(uint16_t gpio);

/**
 * @brief This function is to enable the debounce gpio reset.
 *
 * @param time_ms is threshold of gpio's debounce
 */
void iot_debounce_enable_hard_reset(uint8_t time_ms);

/**
 * @brief This function is to clear for debounce gpio reset.
 */
void iot_debounce_disable_hard_reset(void);

/**
 * @brief This function is to re-config for debounce gpio reset.
 *
 * @param ch is channel.
 * @param rst_time is tidemark of gpio's debounce to re-config.
 */
void iot_debounce_set_time(uint8_t ch, uint8_t rst_time);

/**
 * @brief This function is to clear the reset counter.
 *
 * @param clear_timer_mode decides the interval time to clear the counter.
 */
void iot_debounce_reconfig_hard_reset_time(IOT_DEBOUNCE_CLEAR_TIMER_MODE clear_timer_mode);

/**
 * @brief This function is to clear the reset counter during the coredump with time dalay.
 *
 */
void iot_debounce_reconfig_hard_reset_time_with_delay(void);

/**
 * @brief This function is to get the flag of debounce reset.
 *
 * @return uint8_t is the value of flag.
 */
uint8_t iot_debounce_get_hard_reset_flag(void);

/**
 * @brief This function is to clear the flag of debounce reset.
 */
void iot_debounce_clear_hard_reset_flag(void);

/**
 * @brief Print shutdown fail debug information
 */
void iot_debounce_sleep_debug(void);

/**
 * @brief This function is to set the flag for clearing the counter.
 *
 * @param flag 1 for allowing clearing counter, 0 for stopping clearing counter.
 */
void iot_debounce_set_clr_counter_flag(uint8_t flag);

/**
 * @brief This function is to get the flag for clearing the counter.
 *
 * @return uint8_t is value of the flag.
 */
uint8_t iot_debounce_get_clr_counter_flag(void);

#ifdef __cplusplus
}
#endif
/**
* @}
* @}
*/
#endif /* _DRIVER_NON_OS_DEBOUNCE_H */
