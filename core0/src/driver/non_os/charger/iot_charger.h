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

#ifndef _DRIVER_NON_OS_CHARGER_H
#define _DRIVER_NON_OS_CHARGER_H
/**
 * @addtogroup HAL
 * @{
 * @addtogroup CHARGER
 * @{
 * This section introduces the CHARGER module's enum, structure, functions and how to use this driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define CHG_VOUT_DELTA_300MV_CODE (9)
#define CHG_VOUT_4000MV_CODE      (9)
#define CHG_VOUT_MIN_CODE         (0)
#define CHG_VOUT_MAX_CODE         (31)

/** @defgroup hal_charger_enum Enum
  * @{
  */

/** @brief charger vol.*/
typedef enum {
    IOT_CHARGER_VOL_3734,
    IOT_CHARGER_VOL_3772,
    IOT_CHARGER_VOL_3802,
    IOT_CHARGER_VOL_3832,
    IOT_CHARGER_VOL_3863,
    IOT_CHARGER_VOL_3894,
    IOT_CHARGER_VOL_3925,
    IOT_CHARGER_VOL_3957,
    IOT_CHARGER_VOL_3990,
    IOT_CHARGER_VOL_4018,
    IOT_CHARGER_VOL_4046,
    IOT_CHARGER_VOL_4074,
    IOT_CHARGER_VOL_4103,
    IOT_CHARGER_VOL_4132,
    IOT_CHARGER_VOL_4162,
    IOT_CHARGER_VOL_4192,
    IOT_CHARGER_VOL_4222,
    IOT_CHARGER_VOL_4253,
    IOT_CHARGER_VOL_4284,
    IOT_CHARGER_VOL_4316,
    IOT_CHARGER_VOL_4348,
    IOT_CHARGER_VOL_4381,
    IOT_CHARGER_VOL_4415,
    IOT_CHARGER_VOL_4448,
    IOT_CHARGER_VOL_4483,
    IOT_CHARGER_VOL_4510,
    IOT_CHARGER_VOL_4539,
    IOT_CHARGER_VOL_4567,
    IOT_CHARGER_VOL_4596,
    IOT_CHARGER_VOL_4625,
    IOT_CHARGER_VOL_4655,
    IOT_CHARGER_VOL_4685,
} IOT_CHARGER_VOL;

/** @brief charger interrupt type.*/
typedef enum {
    /**< Disable the interrupt. */
    IOT_CHG_INT_DISABLE,
    /**< Interrupt triggered when switchs from LOW to HIGH. */
    IOT_CHG_INT_EDGE_RAISING,
    /**< Interrupt triggered when switchs from HIGH to LOW. */
    IOT_CHG_INT_EDGE_FALLING,
    /**< Interrupt triggered when switchs to HIGH or LOW . */
    IOT_CHG_INT_EDGE_BOTH,
    /**< Interrupt triggered when stays in LOW. */
    IOT_CHG_INT_LEVEL_LOW,
    /**< Interrupt triggered when stays in HIGH. */
    IOT_CHG_INT_LEVEL_HIGH,
    /**< Invalid value */
    IOT_CHG_INT_MODE_MAX
} IOT_CHARGER_INT_TYPE;

enum {
    IOT_CHG_CC,
    IOT_CHG_CV,
    IOT_CHG_UNKONWN,
};
/**
  * @}
  */

/** @defgroup hal_charger_typedef Typedef
  * @{
 */
typedef void (*iot_charger_int_callback)(uint8_t flag);
/**
  * @}
  */

/**
 * @brief This function is to init PMM Charger.
 *
 */
void iot_charger_init(void);

/**
 * @brief This function is to get flag whether the charging state.
 * @return bool_t bit values of charger_flag_cfg register
 */
bool_t iot_charger_flag_get(void);

/**
 * @brief This function is to register charger's callback.
 *
 * @param int_type is charger interrupt tpye
 * @param cb is charger's callback
 */
void iot_charger_register_int_cb(IOT_CHARGER_INT_TYPE int_type, iot_charger_int_callback cb);

/**
 * @brief This function is to enable charger's gpio.
 *
 * @param enable is enable charger's gpio
 */
void iot_charger_gpio_enable(bool_t enable);

/**
 * @brief set charger max current
 *
 * @param cur_0_1ma mA*10
 */
void iot_charger_set_current(uint16_t cur_0_1ma);

/**
 * @brief set charger max current in deep sleep mode
 *
 * @param cur_0_1ma mA*10
 */
void iot_charger_set_current_dp(uint16_t cur_0_1ma);

/**
 * @brief This function is to set charger's voltage
 *
 * @param vol is charger's voltage
 */
void iot_charger_set_voltage(IOT_CHARGER_VOL vol);

/**
 * @brief This function is to enable charger interrupt
 * @return true or false
 *
 */
void iot_charger_int_enable(void);

/**
 * @brief This function is to clear charger int
 *
 */
void iot_charger_clear_charger_flag_int(void);

/**
 * @brief This function is to disable charger interrupt
 *
 */
void iot_charger_int_disable(void);

/**
 * @brief This function is to get charger's interrupt type: charger on or off state.
 *
 * @return IOT_CHARGER_INT_TYPE charger's interrupt type.
 */
IOT_CHARGER_INT_TYPE iot_charger_get_int_type(void);

/**
 * @brief This function is to get vbat voltage
 * @return vbat voltage mv
 *
 */
uint16_t iot_charger_get_vbat_mv(void);

/**
 * @brief This function is to get vcharger voltage
 * @return vcharger voltage mv
 *
 */
uint16_t iot_charger_get_vcharger_mv(void);

/**
 * @brief This function is to enable charger current monitor
 *
 */
void iot_charger_mon_enable(void);

/**
 * @brief This function is to get charger monitor flag
 * @return true or false
 *
 */
bool_t iot_charger_get_charger_mon_flag(void);

/**
 * @brief This function is clear charger mon flag
 *
 */
void iot_charger_clear_charger_mon_flag(void);

/**
 * @brief This function is to get hw charger cv mode flag
 * @return true or false
 *
 */
uint8_t iot_charger_get_charger_state(void);

#ifdef __cplusplus
}
#endif
/**
* @}
* @}
*/
#endif /* _DRIVER_NON_OS_CHARGER_H */
