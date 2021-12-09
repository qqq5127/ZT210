/****************************************************************************

Copyright(c) 2016 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/

#ifndef OS_SHIM_SYSTICK_H
#define OS_SHIM_SYSTICK_H

/**
 * @addtogroup OS_SHIM
 * @{
 * @addtogroup SYSTICK
 * @{
 * This section introduces the SYSTICK module's functions and how to use this driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup os_shim_systick_typedef Typedef
 * @{
 */
typedef void (*os_systick_init_callback)(uint32_t rate);

typedef void (*os_systick_clear_callback)(void);
/**
 * @}
 */

/**
 * @brief This function is used to init systick port.
 *
 * @param rate is init rate.
 */
void vPortSystickInit(uint32_t rate);

/**
 * @brief This function is used to enable systick port.
 *
 */
void vPortSystickEnable(void);

/**
 * @brief This function is used to clear systick port.
 *
 */
void vPortSysTickClear(void);

/**
 * @brief This function is used to register systick callback.
 *
 * @param init_cb is init callbac.
 * @param clear_cb is clear callback.
 * @return uint8_t RET_OK.
 */
uint8_t os_systick_register_callback(os_systick_init_callback init_cb,
                                     os_systick_clear_callback clear_cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */
#endif /* OS_SHIM_SYSTICK_H */
