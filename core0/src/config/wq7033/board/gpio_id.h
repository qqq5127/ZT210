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

#ifndef CONFIG_BOARD_GPIO_ID_H
#define CONFIG_BOARD_GPIO_ID_H

/* GPIO id generated from MCU Config tool, don't edit this */
typedef enum {
    GPIO_UART0_CTS = 0,
    GPIO_UART0_RXD,
    GPIO_UART0_RTS,
    GPIO_UART0_TXD,
    GPIO_UART1_CTS,
    GPIO_UART1_RXD,
    GPIO_UART1_RTS,
    GPIO_UART1_TXD,
    GPIO_LED_0,
    GPIO_LED_1,
    GPIO_LED_2,
    GPIO_KEY_0,
    GPIO_KEY_1,
    GPIO_KEY_2,
    GPIO_KEY_3,
    GPIO_KEY_4,
    GPIO_KEY_5,
    GPIO_KEY_6,
    GPIO_KEY_7,
    GPIO_KEY_8,
    GPIO_KEY_9,
    GPIO_KEY_10,
    GPIO_POWER_ON,
    GPIO_INCOMING_CALL,
    GPIO_OUTCOMING_CALL,
    GPIO_ACTIVE_CALL,
    GPIO_AUDIO_MUTE,

    GPIO_TOUCH_KEY_0 = 0x60,
    GPIO_TOUCH_KEY_1,

    GPIO_INEAR_KEY_0 = 0x70,
    GPIO_INERA_KEY_1,

    GPIO_CUSTOMIZE_1 = 0x80,
    GPIO_CUSTOMIZE_2,
    GPIO_CUSTOMIZE_3,
    GPIO_CUSTOMIZE_4,
    GPIO_CUSTOMIZE_5,
    GPIO_CUSTOMIZE_6,
    GPIO_CUSTOMIZE_7,
    GPIO_CUSTOMIZE_8,
}RESOURCE_GPIO_ID;

/* MIC id generated from MCU Config tool, don't edit this */
typedef enum {
    RESOURCE_AUDIO_VOICE_MAIN = 0,
    RESOURCE_AUDIO_ANC_FF,
    RESOURCE_AUDIO_ANC_FB,
    RESOURCE_AUDIO_VOICE_SECOND,
    RESOURCE_AUDIO_VOICE_THIRD,
    RESOURCE_AUDIO_VAD,
    RESOURCE_AUDIO_PLAY,
    RESOURCE_AUDIO_MAX,
}RESOURCE_AUDIO_PATH_ID;

typedef enum{
    IOMAP_TYPE_GPIO = 0,
    IOMAP_TYPE_AUDIO,
}RESOURCE_MAP_TYPE;

#define GPIO_MAP_TOUCH_ID(gpio) (((gpio) == 69)? 2:3)

#endif //CONFIG_BOARD_GPIO_ID_H
