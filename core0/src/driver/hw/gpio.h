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
#ifndef _DRIVER_HW_GPIO_H
#define _DRIVER_HW_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_DIG_START GPIO_00
#define GPIO_DIG_MAX GPIO_31

#define GPIO_2DIE_START GPIO_32
#define GPIO_2DIE_MAX GPIO_62

#define GPIO_PMM_START GPIO_63
#define GPIO_PMM_MAX GPIO_70

#define GPIO_ADC_START GPIO_71
#define GPIO_ADC_MAX GPIO_76

#define GPIO_DDR_START GPIO_77
#define GPIO_DDR_MAX GPIO_90

#define GPIO_AON_START GPIO_AON_00
#define GPIO_AON_MAX GPIO_AON_07

typedef enum {
    GPIO_00,    // D GPIO
    GPIO_01,
    GPIO_02,
    GPIO_03,
    GPIO_04,
    GPIO_05,
    GPIO_06,
    GPIO_07,
    GPIO_08,
    GPIO_09,
    GPIO_10,
    GPIO_11,
    GPIO_12,
    GPIO_13,
    GPIO_14,
    GPIO_15,
    GPIO_16,
    GPIO_17,
    GPIO_18,
    GPIO_19,
    GPIO_20,
    GPIO_21,
    GPIO_22,
    GPIO_23,
    GPIO_24,
    GPIO_25,
    GPIO_26,
    GPIO_27,
    GPIO_28,
    GPIO_29,
    GPIO_30,
    GPIO_31,    // DUMMY

    GPIO_32,    // 2-die pad
    GPIO_33,
    GPIO_34,
    GPIO_35,
    GPIO_36,
    GPIO_37,
    GPIO_38,
    GPIO_39,
    GPIO_40,
    GPIO_41,
    GPIO_42,
    GPIO_43,
    GPIO_44,
    GPIO_45,
    GPIO_46,
    GPIO_47,
    GPIO_48,
    GPIO_49,
    GPIO_50,
    GPIO_51,
    GPIO_52,
    GPIO_53,
    GPIO_54,
    GPIO_55,
    GPIO_56,
    GPIO_57,
    GPIO_58,
    GPIO_59,
    GPIO_60,
    GPIO_61,
    GPIO_62,

    GPIO_63,    // pmm gpio
    GPIO_64,
    GPIO_65,
    GPIO_66,
    GPIO_67,    // TK0
    GPIO_68,
    GPIO_69,
    GPIO_70,    // TK4

    GPIO_71,    // meter adc gpio
    GPIO_72,
    GPIO_73,
    GPIO_74,
    GPIO_75,
    GPIO_76,

    GPIO_77,    // ddr pads
    GPIO_78,
    GPIO_79,
    GPIO_80,
    GPIO_81,
    GPIO_82,
    GPIO_83,
    GPIO_84,
    GPIO_85,
    GPIO_86,
    GPIO_87,
    GPIO_88,
    GPIO_89,
    GPIO_90,

    GPIO_AON_00,
    GPIO_AON_01,
    GPIO_AON_02,
    GPIO_AON_03,
    GPIO_AON_04,     // push button
    GPIO_AON_05,
    GPIO_AON_06,
    GPIO_AON_07,

    GPIO_MAX,
} GPIO_ID;

typedef enum {
    GPIO_DIRECTION_INPUT = 0,
    GPIO_DIRECTION_OUTPUT = 1,
} GPIO_DIRECTION;

/**
 * @brief Modes of interrupt. Only when gpio_mode set as GPIO_INTERRUPT,
 * int_mode is available.
 */
typedef enum {
    /**< Disable the interrupt. */
    GPIO_INT_DISABLE,
    /**< Interrupt triggered when switchs from LOW to HIGH. */
    GPIO_INT_EDGE_RAISING,
    /**< Interrupt triggered when switchs from HIGH to LOW. */
    GPIO_INT_EDGE_FALLING,
    /**< Interrupt triggered when switchs to HIGH or LOW . */
    GPIO_INT_EDGE_BOTH,
    /**< Interrupt triggered when stays in LOW. */
    GPIO_INT_LEVEL_LOW,
    /**< Interrupt triggered when stays in HIGH. */
    GPIO_INT_LEVEL_HIGH,
    /**< Invalid value */
    GPIO_INT_MODE_MAX
} GPIO_INT_MODE;

/**
 * @brief
 */
typedef enum {
    GPIO_PULL_NONE,
    GPIO_PULL_UP,
    GPIO_PULL_DOWN,
} GPIO_PULL_MODE;

typedef enum {
    GPIO_DRIVE_LOW,
    GPIO_DRIVE_MEDIUM,
    GPIO_DRIVE_HIGH,
} GPIO_DRIVE_MODE;

uint8_t gpio_claim(uint16_t gpio);
uint8_t gpio_claim_group(const uint16_t gpio[], uint16_t num,
                              bool_t ignore_invalid);
void gpio_release(uint16_t gpio);
void gpio_init(void);
void gpio_deinit(void);
uint8_t gpio_config(uint16_t gpio, GPIO_DIRECTION dir);
uint8_t gpio_open(uint16_t gpio, GPIO_DIRECTION dir);
void gpio_close(uint16_t gpio);
void gpio_write(uint16_t gpio, uint8_t value);
uint8_t gpio_read(uint16_t gpio);
void gpio_toggle(uint16_t gpio);
void gpio_int_enable(uint16_t gpio, GPIO_INT_MODE mode);
void gpio_int_disable(uint16_t gpio);
void gpio_int_disable_all(void);
void gpio_int_clear(uint16_t gpio);
uint32_t gpio_get_int_state(uint16_t gpio);
void gpio_wakeup_enable(uint16_t gpio, bool_t enable);
void gpio_set_pull_mode(uint16_t gpio, GPIO_PULL_MODE mode);
void gpio_get_wakeup_source(uint16_t *gpio, uint8_t *level);
int8_t gpio_set_drive(uint16_t gpio, GPIO_DRIVE_MODE drv);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_GPIO_H */
