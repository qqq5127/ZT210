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

/**
 * @addtogroup APP
 * @{
 */

/**
 * @addtogroup APP_RO_CFG
 * @{
 * This section introduces the APP RO CFG module's enum, structure, functions and how to use this module.
 */

#ifndef __RO_CFG_H__
#define __RO_CFG_H__
#include "types.h"
#include "userapp_dbglog.h"
#include "storage_controller.h"

#define DBGLOG_RO_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[ro] " fmt, ##__VA_ARGS__)
#define DBGLOG_RO_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[ro] " fmt, ##__VA_ARGS__)

/** @defgroup ro_cfg_enum Enum
 * @{
 */

typedef enum {
    /* ID for ro_cfg_general_t*/
    USER_GENERAL_ID = APP_BASE_ID,
    /* ID for ro_cfg_system_t */
    USER_SYSTEM_ID,
    /* ID for reserved usage */
    USER_STORAGE_ID_RESERVED,
    /* ID for ro_cfg_volume_t */
    USER_VOLUME_ID,
    /* ID for ro_cfg_battery_t */
    USER_BATTERY_ID,
    /* ID for ro_cfg_timeout_t */
    USER_TIMER_ID,

    /*ID for ro_cfg_tone_setting_t*/
    USER_TONE_SETTING_ID,
    /* num in ro_cfg_tone_list_t */
    USER_TONE_NUM_ID,
    /* tones in ro_cfg_tone_list_t, uint32 */
    USER_TONE_LIST_ID,
    /* ID for ro_cfg_key_setting_t */

    USER_KEY_SETTING_ID,
    /* num in ro_cfg_key_event_list_t, uint32 */
    USER_EVENT_KEY_NUM_ID,
    /* key_events in ro_cfg_key_event_list_t */
    USER_EVENT_KEY_LIST_ID,

    /* num in ro_cfg_led_list_t for events, uint32*/
    USER_EVENT_LED_NUM_ID,
    /* leds in ro_cfg_led_list_t for events  */
    USER_EVENT_LED_LIST_ID,

    /* num in ro_cfg_led_list_t for states, uint32*/
    USER_STATE_LED_NUM_ID,
    /* leds in ro_cfg_led_list_t for states */
    USER_STATE_LED_LIST_ID,

} USER_STORAGE_ID;

/**
 * @}
 */

#pragma pack(push)
#pragma pack(1)

/** @defgroup ro_cfg_struct Struct
 * @{
 */
typedef struct {
    /* IC part name */
    uint8_t ic_part_name[16];
    uint8_t reserved1[42];
    /* customer assigned firmware version */
    uint32_t fw_version;
    /* customer assigned vendor ID */
    uint32_t vid;
    /* customer assigned product ID */
    uint32_t pid;
    enum {
        RO_LISTEN_MODE_NORMAL,
        RO_LISTEN_MODE_ANC,
        RO_LISTEN_MODE_TRANSPARENCY,
    } default_listen_mode : 8;
    enum {
        RO_LISTEN_MODE_TOGGLE_NONE = 0,
        RO_LISTEN_MODE_TOGGLE_NORMAL = 1,
        RO_LISTEN_MODE_TOGGLE_ANC = 2,
        RO_LISTEN_MODE_TOGGLE_NORMAL_ANC = 3,
        RO_LISTEN_MODE_TOGGLE_TRANSPARENCY = 4,
        RO_LISTEN_MODE_TOGGLE_NORMAL_TRANSPARENCY = 5,
        RO_LISTEN_MODE_TOGGLE_ANC_TRANSPARENCY = 6,
        RO_LISTEN_MODE_TOGGLE_NORMAL_ANC_TRANSPARENCY = 7,
        RO_LISTEN_MODE_TOGGLE_NORMAL_TRANSPARENCY_ANC = 8,
    } default_listen_mode_toggle_config : 8;
    uint8_t reserved2[16];
} ro_cfg_general_t;

typedef struct {
    uint8_t reserved1[8];
    /* auto reconnect retry count after power on */
    uint16_t power_on_reconnect_retry_count;
    /* link loss reconnect retry count */
    uint16_t link_loss_reconnect_retry_count;
    uint8_t reserved3[26];
    uint8_t reserved7;
    uint8_t reserved8[19];
    /* feature enabled */
    struct ro_cfg_features {
        /* auto power on/off enabled */
        uint32_t auto_power_off : 1;
        /* sync system state with master */
        uint32_t sync_sys_state : 1;
        /* stereo mode or tws mode */
        uint32_t stereo_mode : 1;
        uint32_t reserved1 : 3;
        /* queue tone enabled */
        uint32_t queue_tone : 1;
        /* SSP enabled */
        uint32_t reserved2 : 2;
        /* discoverable all time */
        uint32_t reserved3 : 1;
        /* discoverable after power on */
        uint32_t reserved4 : 1;
        /* discoverable after disconnected */
        uint32_t discoverable_disconnect : 1;
        /* discoverable if PDL empty */
        uint32_t reserved5 : 1;
        /* discoverable during link loss */
        uint32_t reserved6 : 1;
        /* auto reconnection enabled */
        uint32_t auto_reconnect_power_on : 1;
        uint32_t reserved : 17;
    } features;
    uint8_t reserved5[16];
} ro_cfg_system_t;

typedef struct {
    /* default music volume level */
    uint8_t default_music_volume;
    /* default call volume level */
    uint8_t default_call_volume;
    /* default TONE volume */
    uint8_t default_tone_volume;
    /* default MIC gain */
    int8_t default_mic_gain;
    /* num of volume level */
    uint8_t volume_level_num;
    /* list of volume level */
    struct ro_cfg_volume_level {
        /* gain of music */
        int8_t music_gain;
        /* gain of call */
        int8_t call_gain;
        /* gain of tone */
        int8_t tone_gain;
    } volume_levels[16];
    /* reserved1 */
    uint8_t reserved1;
    /* reserved2 */
    uint8_t reserved2;
    /* reserved3 */
    uint8_t reserved3;
} ro_cfg_volume_t;

typedef struct {
    /* voltage in mV to trigger battery low event */
    uint16_t battery_low_event_voltage;
    /* voltage in mV to trigger battery low power off event */
    uint16_t battery_low_power_off_voltage;
    /* cyclic timer to play warning tone and led when battery low */
    uint8_t battery_low_prompt_interval;
    /* cyclic timer to check current battery level */
    uint8_t battery_level_check_interval;
    /* voltage in mV of 10%, 20%, 30%, ... 100% */
    uint16_t battery_level_voltages[10];
    /* high byte of battery_low_prompt_interval */
    uint8_t battery_low_prompt_interval_high;
    /* reserved1 */
    uint8_t reserved1;
} ro_cfg_battery_t;

typedef struct {
    uint8_t reserved1[16];
    /* auto power off timeout */
    uint32_t auto_power_off;
    /* timeout to disable power off after power on */
    uint32_t disable_power_off_after_power_on;
    /* power on reconnect interval period */
    uint32_t power_on_reconnect_interval;
    /* link loss reconnect interval period */
    uint32_t link_loss_reconnect_interval;
    uint8_t reserved3[28];
} ro_cfg_timeout_t;

typedef struct {
    /* 0:independent or 1:follow streaming 2:other*/
    uint8_t tone_volume_mode;
    /* default language*/
    uint8_t tone_language;
} ro_cfg_tone_setting_t;

typedef struct {
    /* tone num */
    uint32_t num;
    /* tone list */
    struct ro_cfg_tone {
        /* event ID */
        uint32_t event_id;
        /* tone ID */
        uint8_t tone_id;
        /* 0:left/1right/2both/3self */
        uint8_t tone_channel;
        /*volume level in tone independce mode*/
        uint8_t tone_volume;
    } tones[];
} ro_cfg_tone_list_t;

typedef struct {
    /* long press time */
    uint16_t long_time;
    /* very long press time */
    uint16_t vlong_time;
    /* very very long press time */
    uint16_t vvlong_time;
    uint16_t reserved1;
    /* climbing edge threshold */
    uint32_t climb_thres;
    /* falling edge threshold */
    uint32_t fall_thres;
    /* Start time after that repeat counted */
    uint16_t repeat_start_time;
    /* repeat rate */
    uint16_t repeat_rate;
    /* maximum interval between continuous twice taps in one Multi-tap event. */
    uint8_t max_multi_tap_interval;
    /* interval between debounce check */
    uint8_t debounce_check_interval;
    /* high 8 bit of interval between debounce check */
    uint8_t max_multi_tap_interval_high;
    uint8_t reserved[6];
} ro_cfg_key_setting_t;

typedef struct {
    /* Num of user key event */
    uint32_t num;
    /* List of user key event */
    struct ro_cfg_key_event {
        /* key related info */
        struct ro_cfg_key_press_info {
            /* key ID and type */
            struct key_id_type {
                /* IO_ID */
                uint8_t id : 6;
                /* IO_source. 0: simple IO, 1: touch 2:external*/
                uint8_t src : 2;
                /* type, defined in T_KeyPressedType_E */
                uint8_t type;
            } keys[3];
            /* key num */
            uint8_t pressed_num : 2;
            /* reserved */
            uint8_t reserved : 6;
        } pressed_info;
        /* user event id */
        uint16_t event;
        /* system BT state bit mask */
        uint16_t system_state;
    } key_events[];
} ro_cfg_key_event_list_t;

/*

 ps:
       <  on  ><off>            < repeat_delay  >
        _______    _______                          ______    _____                          ______    ______
_______|       |__|       |__ _____________________|      |__|     |__ _____________________|      |__|      |__
on              = 200 ms
off             = 50 ms
repeat_delay    = 500 ms
flash           = 2
loop    = 3
dim time for branch mode

*/

// LED
typedef struct {
    uint32_t num; /* number of state led */
    /* LED config */
    struct ro_cfg_led {
        /* state mask or event ID */
        uint32_t state_or_event;
        /* led style */
        struct ro_cfg_led_type {
            /* led work mode */
            enum {
                RO_LED_MODE_OFF,          /*turn off led*/
                RO_LED_MODE_BLINK,        /*blink mode*/
                RO_LED_MODE_DIM,          /*breath mode*/
                RO_LED_MODE_NORMAL_LIGHT, /*normal light mode*/
            } mode;
            /* on time ms */
            uint32_t on;
            /* on time in ms */
            uint32_t off;
            /* 1-start from on, 0-start from off */
            uint32_t start_from_on;
            /* number of flashes */
            uint32_t flash;
            /* delay time in ms when repeat */
            uint32_t repeat_delay;
            /* dim time in ms */
            uint32_t dim;
            /* loop counts */
            uint32_t loop;
            /* LED id */
            uint8_t id;
        } style;
    } leds[];
} ro_cfg_led_list_t;

#pragma pack(pop)

typedef struct {
    const ro_cfg_general_t *general;
    const ro_cfg_system_t *system;
    const ro_cfg_volume_t *volume;
    const ro_cfg_battery_t *battery;
    const ro_cfg_timeout_t *timeout;
    const ro_cfg_tone_setting_t *tone_setting;
    const ro_cfg_tone_list_t *tone;
    const ro_cfg_key_setting_t *key_setting;
    const ro_cfg_key_event_list_t *key_event;
    const ro_cfg_led_list_t *event_led;
    const ro_cfg_led_list_t *state_led;
} ro_cfg_t;

/**
 * @}
 */

/**
 * @brief get the readonly configurations
 *
 * @return the readonly configurations
 */
const ro_cfg_t *ro_cfg(void);

/**
 * @brief get the readonly general configurations
 *
 * @return the readonly general configurations
 */
const ro_cfg_general_t *ro_gen_cfg(void);

/**
 * @brief get the readonly system configurations
 *
 * @return the readonly system configurations
 */
const ro_cfg_system_t *ro_sys_cfg(void);

/**
 * @brief get the readonly volume configurations
 *
 * @return the readonly volume configurations
 */
const ro_cfg_volume_t *ro_vol_cfg(void);

/**
 * @brief get the readonly battery configurations
 *
 * @return the readonly battery configurations
 */
const ro_cfg_battery_t *ro_bat_cfg(void);

/**
 * @brief get the readonly timeout configurations
 *
 * @return the readonly timeout configurations
 */
const ro_cfg_timeout_t *ro_to_cfg(void);

/**
 * @brief get the readonly features configurations
 *
 * @return the readonly features configurations
 */
const struct ro_cfg_features *ro_feat_cfg(void);

/**
 * @brief init readonly configuration module
 */
void ro_cfg_init(void);

/**
 * @brief deinit readonly configuration module
 */
void ro_cfg_deinit(void);

/**
 * @brief release the key_setting data from readonly configuration,
 *        the key_setting in ro_cfg_t will be not accessible after
 *        this function is called
 */
void ro_cfg_release_key_setting(void);

/**
 * @}
 * addtogroup APP_RO_CFG
 */

/**
 * @}
 * addtogroup APP
 */

#endif
