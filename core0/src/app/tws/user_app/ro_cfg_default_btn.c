#include "types.h"
#include "os_mem.h"
#include "string.h"
#include "modules.h"
#include "key_sensor.h"
#include "ro_cfg.h"
#include "app_bt.h"
#include "app_evt.h"

#define KEY_TO_ACTION_TOTAL_NUM 20
const struct ro_cfg_key_event
    key_to_user[KEY_TO_ACTION_TOTAL_NUM] =
        {
            {
                .pressed_info =
                    {
                        .keys = {{.id = 1, .type = BTN_TYPE_SINGLE},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_MUSIC_PLAY,
                .system_state = STATE_CONNECTED,
            },

            {
                .pressed_info =
                    {
                        .keys = {{.id = 1, .type = BTN_TYPE_SINGLE},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_MUSIC_PAUSE,
                .system_state = STATE_A2DP_STREAMING,
            },

            {
                .pressed_info =
                    {
                        .keys = {{.id = 1, .type = BTN_TYPE_SINGLE},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_ENTER_PAIRING,
                .system_state = STATE_IDLE | STATE_CONNECTABLE,
            },

            {
                .pressed_info =
                    {
                        .keys = {{.id = 1, .type = BTN_TYPE_DOUBLE},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_MUSIC_BACKWARD,
                .system_state = STATE_A2DP_STREAMING,
            },

            {
                .pressed_info =
                    {
                        .keys = {{.id = 1, .type = BTN_TYPE_TRIPLE},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_MUSIC_FORWARD,
                .system_state = STATE_A2DP_STREAMING,
            },

            {
                .pressed_info =
                    {
                        .keys = {{.id = 1, .type = BTN_TYPE_SINGLE},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_ANSWER,
                .system_state = STATE_INCOMING_CALL,
            },

            {
                .pressed_info =
                    {
                        .keys = {{.id = 1, .type = BTN_TYPE_SINGLE},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_HANGUP,
                .system_state = STATE_ACTIVE_CALL | STATE_TWC_CALL_ON_HELD | STATE_OUTGOING_CALL,
            },

            {
                .pressed_info =
                    {
                        .keys = {{.id = 1, .type = BTN_TYPE_DOUBLE},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_REJECT,
                .system_state = STATE_INCOMING_CALL,
            },

            {
                .pressed_info =
                    {
                        .keys = {{.id = 1, .type = BTN_TYPE_DOUBLE},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_REDIAL_LAST,
                .system_state = STATE_CONNECTED,
            },

            {
                .pressed_info =
                    {
                        .keys = {{.id = 1, .type = BTN_TYPE_QUADRUPLE},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_ENTER_OTA_MODE,
                .system_state = STATE_IDLE | STATE_AG_PAIRING | STATE_CONNECTED | STATE_CONNECTABLE,
            },

            {
                .pressed_info =
                    {
                        .keys = {{.id = 1, .type = BTN_TYPE_QUINTUPLE},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_ENTER_FACTORY_TEST_MODE,
                .system_state = STATE_IDLE | STATE_AG_PAIRING | STATE_CONNECTED | STATE_CONNECTABLE,
            },

            {
                .pressed_info =
                    {
                        .keys = {{.id = 1, .type = BTN_TYPE_LONG},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_POWER_ON,
                .system_state = STATE_DISABLED,
            },

            {
                .pressed_info =
                    {
                        .keys = {{.id = 1, .type = BTN_TYPE_LONG},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_POWER_OFF,
                .system_state = 0xFFFF & (~STATE_DISABLED),
            },

            {
                .pressed_info =
                    {
                        .keys = {{.id = 2, .type = BTN_TYPE_SINGLE},
                                 {.id = 0, .type = BTN_LAST},
                                 {.id = 0, .type = BTN_LAST}},
                        .pressed_num = 1,
                        .reserved = 0,
                    },
                .event = EVTUSR_WWS_ROLE_SWITCH,
                .system_state = 0xFFFF & (~STATE_DISABLED),
            },
};

void ro_cfg_default_key_event(ro_cfg_key_event_list_t **key_event);

void ro_cfg_default_key_event(ro_cfg_key_event_list_t **ret_key_event)
{
    ro_cfg_key_event_list_t *key_event;
    uint32_t len;
    uint32_t num;

    num = KEY_TO_ACTION_TOTAL_NUM;
    len = sizeof(struct ro_cfg_key_event) * num;
    key_event = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_key_event_list_t) + len);
    key_event->num = num;

    memcpy_s(key_event->key_events, len, key_to_user, sizeof(key_to_user));

    *ret_key_event = key_event;
}

void ro_cfg_default_key_setting(ro_cfg_key_setting_t *key_setting);

void ro_cfg_default_key_setting(ro_cfg_key_setting_t *key_setting)
{
    key_setting->long_time = 1500;
    key_setting->vlong_time = 4000;
    key_setting->vvlong_time = 8000;
    key_setting->climb_thres = 151920;
    key_setting->fall_thres = 151280;
    key_setting->repeat_start_time = 1500;
    key_setting->repeat_rate = 400;
    key_setting->max_multi_tap_interval = 200;
    key_setting->debounce_check_interval = 20;
}
