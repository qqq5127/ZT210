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
#ifndef _DRIVER_HW_PIN_H
#define _DRIVER_HW_PIN_H

#ifdef __cplusplus
extern "C" {
#endif

#define PIN_PMM_START    PIN_63
#define PIN_PMM_END      PIN_70
#define PIN_ADC_0        PIN_71
#define PIN_ADC_1        PIN_72
#define PIN_ADC_2        PIN_73
#define PIN_ADC_3        PIN_74
#define PIN_DIG_MTX_FUNC 0
#define PIN_PMM_MTX_FUNC 3
#define PIN_PMM_AON_FUNC 0

typedef enum {
    PIN_0,   // GPIO00
    PIN_1,
    PIN_2,
    PIN_3,
    PIN_4,
    PIN_5,
    PIN_6,
    PIN_7,
    PIN_8,
    PIN_9,
    PIN_10,
    PIN_11,
    PIN_12,
    PIN_13,
    PIN_14,
    PIN_15,
    PIN_16,
    PIN_17,
    PIN_18,
    PIN_19,
    PIN_20,
    PIN_21,
    PIN_22,
    PIN_23,
    PIN_24,
    PIN_25,
    PIN_26,
    PIN_27,
    PIN_28,
    PIN_29,
    PIN_30,
    PIN_31,   // GPIO31

    PIN_32,   // ADI_D_GPIO00
    PIN_33,
    PIN_34,
    PIN_35,
    PIN_36,
    PIN_37,
    PIN_38,
    PIN_39,
    PIN_40,
    PIN_41,
    PIN_42,
    PIN_43,
    PIN_44,
    PIN_45,
    PIN_46,
    PIN_47,
    PIN_48,
    PIN_49,
    PIN_50,
    PIN_51,
    PIN_52,
    PIN_53,
    PIN_54,
    PIN_55,
    PIN_56,
    PIN_57,
    PIN_58,
    PIN_59,
    PIN_60,
    PIN_61,
    PIN_62,   // ADI_D_GPIO30

    PIN_63,   // PMU_GPIO00
    PIN_64,
    PIN_65,
    PIN_66,
    PIN_67,
    PIN_68,
    PIN_69,
    PIN_70,
    PIN_71,
    PIN_72,
    PIN_73,
    PIN_74,
    PIN_75,   // PMU_GPIO12

    PIN_MAX,
    PIN_NONE = PIN_MAX,
} PIN_ID;

typedef enum {
    PIN_PULL_NONE,
    PIN_PULL_UP,
    PIN_PULL_DOWN,
} PIN_PULL_MODE;

typedef enum {
    PIN_DRV_STRENGTH_LOW,
    PIN_DRV_STRENGTH_MEDIUM,
    PIN_DRV_STRENGTH_HIGH,
} PIN_DRV_STRENGTH;

int8_t pin_claim_as_gpio(uint16_t pin);
int8_t pin_set_func(uint16_t pin, uint8_t func);
int8_t pin_set_drv_strength(uint16_t pin, PIN_DRV_STRENGTH drv);
PIN_DRV_STRENGTH pin_get_drv_strength(uint16_t pin);
int8_t pin_claim(uint16_t pin);
int8_t pin_release(uint16_t pin);
int8_t pin_set_pull_mode(uint16_t pin, PIN_PULL_MODE mode);
uint32_t pin_get_pin_config(uint16_t pin);
#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_PIN_H */
