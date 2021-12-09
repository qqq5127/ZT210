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

#ifndef _DRIVER_NON_OS_CLOCK_H
#define _DRIVER_NON_OS_CLOCK_H

/**
 * @addtogroup HAL
 * @{
 */

/**
 * @addtogroup CLOCK
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum IOT_CLOCK_CORE
 * The possible frequency of each cpu
 */
typedef enum {
    IOT_CLOCK_CORE_16M = 1,
    IOT_CLOCK_CORE_32M,
    IOT_CLOCK_CORE_48M,
    IOT_CLOCK_CORE_64M,
    IOT_CLOCK_CORE_80M,
    IOT_CLOCK_CORE_96M,
    IOT_CLOCK_CORE_128M = 8,
    IOT_CLOCK_CORE_160M = 10,
    IOT_CLOCK_CORE_MAX,
} IOT_CLOCK_CORE;

/**
 * @enum IOT_CLOCK_MODE
 * Possible clock combination modes of the system
 */
typedef enum {
    IOT_CLOCK_MODE_0,
    IOT_CLOCK_MODE_1,
    IOT_CLOCK_MODE_2,
    IOT_CLOCK_MODE_3,
    IOT_CLOCK_MODE_4,
    IOT_CLOCK_MODE_5,
    IOT_CLOCK_MODE_6,
    IOT_CLOCK_MODE_7,
    IOT_CLOCK_MODE_8,
    IOT_CLOCK_MODE_9,
    IOT_CLOCK_MODE_10,
    IOT_CLOCK_MODE_11,
    IOT_CLOCK_MODE_12,
    IOT_CLOCK_MODE_13,
    IOT_CLOCK_MODE_14,
    IOT_CLOCK_MODE_15,
    IOT_CLOCK_MODE_16,
    IOT_CLOCK_MODE_17,
    IOT_CLOCK_MODE_18,
    IOT_CLOCK_MODE_19,
    IOT_CLOCK_MODE_20,
    IOT_CLOCK_MODE_21,
    IOT_CLOCK_MODE_22,
    IOT_CLOCK_MODE_23,

    IOT_CLOCK_MODE_MAX,
} IOT_CLOCK_MODE;

/**
 * @brief Reset clock state to 16Mhz, especially sleep when cpu is not 16Mhz.
 */
void iot_clock_reset_state(void);

/**
 * @brief Set the system to run in the specified clock combination mode,
 * will not change peripherals clock.
 *
 * @param[in] mode The system clock mode.
 */
void iot_clock_set_mode(IOT_CLOCK_MODE mode);

void iot_reset_clock_state(void);

/**
 * @brief Get specified cpu core clock by clock mode
 *
 * @param[in] mode The system clock mode.
 * @param[in] core_id The cpu core ID.
 *
 * return IOT clock core
 */
IOT_CLOCK_CORE iot_clock_get_core_clock_by_mode(IOT_CLOCK_MODE mode, uint32_t core_id);


/**
 * @brief Get clock mode by specified clock of the three cpu cores.
 *
 * @param[in] dtop_core_clock clock of DTOP core
 * @param[in] bt_core_clock clock of BT core
 * @param[in] dsp_core_clock clock of DSP core
 *
 * return IOT clock mode
 */
IOT_CLOCK_MODE iot_clock_get_mode_by_core_clock(IOT_CLOCK_CORE dtop_core_clock,
                                                IOT_CLOCK_CORE bt_core_clock,
                                                IOT_CLOCK_CORE dsp_core_clock);
#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup CLOCK
 */

/**
* @}
 * addtogroup HAL
*/

#endif /* _DRIVER_NON_OS_CLOCK_H */
