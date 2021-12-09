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

#ifndef _DRIVER_NON_OS_GPIO_H
#define _DRIVER_NON_OS_GPIO_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup GPIO
 * @{
 * This section introduces the GPIO module's enum, structure, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IOT_GPIO_MAX_NUM 90

/**
 * @defgroup hal_gpio_enum Enum
 * @{
 */

/** @brief GPIO id. */
typedef enum {
    IOT_GPIO_00,    // D GPIO
    IOT_GPIO_01,
    IOT_GPIO_02,
    IOT_GPIO_03,
    IOT_GPIO_04,
    IOT_GPIO_05,
    IOT_GPIO_06,
    IOT_GPIO_07,
    IOT_GPIO_08,
    IOT_GPIO_09,
    IOT_GPIO_10,
    IOT_GPIO_11,
    IOT_GPIO_12,
    IOT_GPIO_13,
    IOT_GPIO_14,
    IOT_GPIO_15,
    IOT_GPIO_16,
    IOT_GPIO_17,
    IOT_GPIO_18,
    IOT_GPIO_19,
    IOT_GPIO_20,
    IOT_GPIO_21,
    IOT_GPIO_22,
    IOT_GPIO_23,
    IOT_GPIO_24,
    IOT_GPIO_25,
    IOT_GPIO_26,
    IOT_GPIO_27,
    IOT_GPIO_28,
    IOT_GPIO_29,
    IOT_GPIO_30,
    IOT_GPIO_31,    // DUMMY

    IOT_GPIO_32,    // 2-die pad
    IOT_GPIO_33,
    IOT_GPIO_34,
    IOT_GPIO_35,
    IOT_GPIO_36,
    IOT_GPIO_37,
    IOT_GPIO_38,
    IOT_GPIO_39,
    IOT_GPIO_40,
    IOT_GPIO_41,
    IOT_GPIO_42,
    IOT_GPIO_43,
    IOT_GPIO_44,
    IOT_GPIO_45,
    IOT_GPIO_46,
    IOT_GPIO_47,
    IOT_GPIO_48,
    IOT_GPIO_49,
    IOT_GPIO_50,
    IOT_GPIO_51,
    IOT_GPIO_52,
    IOT_GPIO_53,
    IOT_GPIO_54,
    IOT_GPIO_55,
    IOT_GPIO_56,
    IOT_GPIO_57,
    IOT_GPIO_58,
    IOT_GPIO_59,
    IOT_GPIO_60,
    IOT_GPIO_61,
    IOT_GPIO_62,

    IOT_GPIO_63,    // pmm gpio
    IOT_GPIO_64,
    IOT_GPIO_65,
    IOT_GPIO_66,
    IOT_GPIO_67,    // tk gpi00
    IOT_GPIO_68,
    IOT_GPIO_69,
    IOT_GPIO_70,    // tk gpi03

    IOT_GPIO_71,    // meter adc gpio
    IOT_GPIO_72,
    IOT_GPIO_73,
    IOT_GPIO_74,
    IOT_GPIO_75,
    IOT_GPIO_76,

    IOT_GPIO_77,    // ddr pads
    IOT_GPIO_78,
    IOT_GPIO_79,
    IOT_GPIO_80,
    IOT_GPIO_81,
    IOT_GPIO_82,
    IOT_GPIO_83,
    IOT_GPIO_84,
    IOT_GPIO_85,
    IOT_GPIO_86,
    IOT_GPIO_87,
    IOT_GPIO_88,
    IOT_GPIO_89,
    IOT_GPIO_90,
    IOT_GPIO_MAX,

    IOT_AONGPIO_00 = IOT_GPIO_MAX,
    IOT_AONGPIO_01,
    IOT_AONGPIO_02,
    IOT_AONGPIO_03,
    IOT_AONGPIO_04,     // push button
    IOT_AONGPIO_05,
    IOT_AONGPIO_06,
    IOT_AONGPIO_07,
    IOT_AONGPIO_08,
    IOT_AONGPIO_MAX,

    IOT_GPIO_NUM = IOT_AONGPIO_MAX,
    IOT_GPIO_INVALID = 0xFF,
} IOT_GPIO_ID;

/** @brief GPIO direction. */
typedef enum {
    IOT_GPIO_DIRECTION_INPUT = 0,
    IOT_GPIO_DIRECTION_OUTPUT = 1,
} IOT_GPIO_DIRECTION;

/** @brief GPIO Modes of interrupt. Only when gpio_mode set as GPIO_INTERRUPT,int_mode is available. */
typedef enum {
    /**< Disable the interrupt. */
    IOT_GPIO_INT_DISABLE,
    /**< Interrupt triggered when switchs from LOW to HIGH. */
    IOT_GPIO_INT_EDGE_RAISING,
    /**< Interrupt triggered when switchs from HIGH to LOW. */
    IOT_GPIO_INT_EDGE_FALLING,
    /**< Interrupt triggered when switchs to HIGH or LOW . */
    IOT_GPIO_INT_EDGE_BOTH,
    /**< Interrupt triggered when stays in LOW. */
    IOT_GPIO_INT_LEVEL_LOW,
    /**< Interrupt triggered when stays in HIGH. */
    IOT_GPIO_INT_LEVEL_HIGH,
    /**< Invalid value */
    IOT_GPIO_INT_MODE_MAX
} IOT_GPIO_INT_MODE;

/** @brief GPIO pull mode. */
typedef enum {
    IOT_GPIO_PULL_NONE,
    IOT_GPIO_PULL_UP,
    IOT_GPIO_PULL_DOWN,
} IOT_GPIO_PULL_MODE;

/** @brief Gpio drive mode */
typedef enum {
    IOT_GPIO_DRIVE_LOW,
    IOT_GPIO_DRIVE_MEDIUM,
    IOT_GPIO_DRIVE_HIGH,
} IOT_GPIO_DRIVE_MODE;

/**
 * @}
 */

/**
 * @defgroup hal_gpio_struct Struct
 * @{
 */
/** GPIO output configuration */
typedef struct gpio_out_config {
    IOT_GPIO_PULL_MODE pull;
} gpio_out_config_t;

/** GPIO input configuration */
typedef struct gpio_in_config {
    IOT_GPIO_PULL_MODE pull;
} gpio_in_config_t;
/**
 * @}
 */

/**
 * @defgroup hal_gpio_typedef Typedef
 * @{
 */
typedef void (*iot_gpio_int_callback)(void);
/**
 * @}
 */

/**
 * @brief Save all gpio state
 *
 * @param data saved data
 * @return uint32_t RET_OK for succ otherwise fail
 */
uint32_t iot_gpio_save(uint32_t data);

/**
 * @brief Restore all gpio state
 *
 * @param data saved data
 * @return uint32_t RET_OK for succ otherwise fail
 */
uint32_t iot_gpio_restore(uint32_t data);

/**
 * @brief This function is to initialise the gpio driver module.
 *
 */
void iot_gpio_init(void);

/**
 * @brief This function is to de-initialise the GPIO module.
 *
 */
void iot_gpio_deinit(void);

/**
 * @brief This function is to open a GPIO for either output or input.
 *
 * @param gpio is the gpio to use.
 * @param dir is input or output direction.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_gpio_open(uint16_t gpio, IOT_GPIO_DIRECTION dir);

/**
 * @brief This function is to open gpio and let it as interrupt.
 *
 * @param gpio is the gpio to use.
 * @param mode is the gpio interrupt mode.
 * @param cb is gpio callback interrupt function.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_gpio_open_as_interrupt(uint16_t gpio, IOT_GPIO_INT_MODE mode,
                                  iot_gpio_int_callback cb);

/**
 * @brief This function is to enable the gpio interrupt.
 *
 * @param gpio is the gpio to use.
 */
void iot_gpio_int_enable(uint16_t gpio);

/**
 * @brief This function is to disable the gpio interrupt.
 *
 * @param gpio is the gpio to use.
 */
void iot_gpio_int_disable(uint16_t gpio);

/**
 * @brief This function is to close a GPIO previously opened.
 *
 * @param gpio is the gpio to use.
 */
void iot_gpio_close(uint16_t gpio);

/**
 * @brief This function is to write data through gpio.
 *
 * @param gpio is the gpio to use.
 * @param value is the data to write.
 */
void iot_gpio_write(uint16_t gpio, uint8_t value);

/**
 * @brief This function is to read data through gpio.
 *
 * @param gpio is the gpio to use.
 * @return uint8_t is the readed data.
 */
uint8_t iot_gpio_read(uint16_t gpio);

/**
 * @brief This function is to toggle the gpio.
 *
 * @param gpio is the gpio to use.
 */
void iot_gpio_toggle(uint16_t gpio);

/**
 * @brief This function is to set gpio pull mode.
 *
 * @param gpio is the gpio to use.
 * @param mode is the gpio pull mode.
 */
void iot_gpio_set_pull_mode(uint16_t gpio, IOT_GPIO_PULL_MODE mode);

/**
 * @brief This function is to get gpio wakeup source's information.
 *
 * @param [out] gpio is wakeup gpio number;
 * @param [out] level is wakeup gpio's low/high state.
 */
void iot_gpio_get_wakeup_source(uint16_t *gpio, uint8_t *level);

/**
 * @brief Set GPIO drive capability
 *
 * @param gpio The gpio number
 * @param drv  Drive mode
 * @return int8_t RET_OK for success otherwise fail
 */
int8_t iot_gpio_set_drive(uint16_t gpio, IOT_GPIO_DRIVE_MODE drv);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */
#endif /* _DRIVER_NON_OS_GPIO_H */
