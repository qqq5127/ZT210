#include "types.h"
#include "os_mem.h"
#include "string.h"
#include "modules.h"
#include "ro_cfg.h"
#include "app_bt.h"
#include "app_evt.h"

#define LED_STATE_NUM 6
#define LED_EVENT_NUM 6

const struct ro_cfg_led led_state[LED_STATE_NUM] = {
    {
        STATE_DISABLED,   //state
        {
            .mode = RO_LED_MODE_DIM,
            .id = 0,
            .on = 0,
            .off = 1000,
            .start_from_on = 0,
            .flash = 1,            // number of flashes
            .repeat_delay = 100,   // delay time in ms when repeat
            .dim = 2000,
            .loop = 0,   // 0 = always
        },
    },
    {
        STATE_CONNECTABLE,   //state
        {
            .mode = RO_LED_MODE_DIM,
            .id = 0,
            .on = 0,
            .off = 1000,
            .start_from_on = 0,
            .flash = 1,            // number of flashes
            .repeat_delay = 100,   // delay time in ms when repeat
            .dim = 2000,
            .loop = 0,   // 0 = always
        },
    },

    {
        STATE_AG_PAIRING,   //state
        {
            .mode = RO_LED_MODE_BLINK,
            .id = 0,
            .on = 100,
            .off = 100,
            .start_from_on = 0,
            .flash = 1,          // number of flashes
            .repeat_delay = 0,   // delay time in ms when repeat
            .dim = 0,
            .loop = 0,
        },
    },

    {
        STATE_CONNECTED,   //state
        {
            .mode = RO_LED_MODE_DIM,
            .id = 0,
            .on = 1000,
            .off = 1000,
            .start_from_on = 0,
            .flash = 1,            // number of flashes
            .repeat_delay = 200,   // delay time in ms when repeat
            .dim = 2000,
            .loop = 0,
        },
    },

    {
        STATE_INCOMING_CALL,   //state
        {
            .mode = RO_LED_MODE_BLINK,
            .id = 0,
            .on = 300,
            .off = 300,
            .start_from_on = 1,
            .flash = 2,            // number of flashes
            .repeat_delay = 400,   // delay time in ms when repeat
            .dim = 0,
            .loop = 0,
        },
    },
    {
        STATE_WWS_PAIRING,   //state
        {
            .mode = RO_LED_MODE_BLINK,
            .id = 0,
            .on = 50,
            .off = 100,
            .start_from_on = 1,
            .flash = 2,            // number of flashes
            .repeat_delay = 150,   // delay time in ms when repeat
            .dim = 0,
            .loop = 0,
        },
    },

};

const struct ro_cfg_led led_event[LED_EVENT_NUM] = {
    {
        EVTUSR_POWER_ON,   //event id
        {
            .mode = RO_LED_MODE_NORMAL_LIGHT,
            .id = 0,
            .on = 0,
            .off = 0,
            .start_from_on = 1,
            .flash = 0,          // number of flashes
            .repeat_delay = 0,   // delay time in ms when repeat
            .dim = 0,
            .loop = 0x1,
        },
    },

    {
        EVTUSR_VOLUME_UP,   //event id
        {
            .mode = RO_LED_MODE_BLINK,
            .id = 0,
            .on = 100,
            .off = 100,
            .start_from_on = 1,
            .flash = 1,          // number of flashes
            .repeat_delay = 0,   // delay time in ms when repeat
            .dim = 0,
            .loop = 0x1,
        },
    },

    {
        EVTUSR_VOLUME_DOWN,   //event ids
        {
            .mode = RO_LED_MODE_BLINK,
            .id = 0,
            .on = 200,
            .off = 200,
            .start_from_on = 1,
            .flash = 1,          // number of flashes
            .repeat_delay = 0,   // delay time in ms when repeat
            .dim = 0,
            .loop = 0x1,
        },
    },

    {
        EVTSYS_CHARGE_CONNECTED,   //event ids
        {
            .mode = RO_LED_MODE_BLINK,
            .id = 0,
            .on = 100,
            .off = 100,
            .start_from_on = 0,
            .flash = 2,          // number of flashes
            .repeat_delay = 0,   // delay time in ms when repeat
            .dim = 0,
            .loop = 0x1,
        },
    },
    {
        EVTSYS_CHARGE_DISCONNECTED,   //event ids
        {
            .mode = RO_LED_MODE_BLINK,
            .id = 0,
            .on = 100,
            .off = 100,
            .start_from_on = 0,
            .flash = 3,          // number of flashes
            .repeat_delay = 0,   // delay time in ms when repeat
            .dim = 0,
            .loop = 0x1,
        },
    },
    {
        EVTSYS_WWS_ROLE_SWITCH,   //event ids
        {
            .mode = RO_LED_MODE_BLINK,
            .id = 0,
            .on = 50,
            .off = 50,
            .start_from_on = 0,
            .flash = 10,         // number of flashes
            .repeat_delay = 0,   // delay time in ms when repeat
            .dim = 0,
            .loop = 0x1,
        },
    }

};

void ro_cfg_default_state_led(ro_cfg_led_list_t **state_led);

void ro_cfg_default_state_led(ro_cfg_led_list_t **ret_state_led)
{
    ro_cfg_led_list_t *state_led;
    uint32_t len;
    uint32_t num;

    num = LED_STATE_NUM;
    len = sizeof(struct ro_cfg_led) * num;
    state_led = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_led_list_t) + len);
    state_led->num = num;
    memcpy_s(state_led->leds, len, led_state, sizeof(led_state));
    *ret_state_led = state_led;
}

void ro_cfg_default_event_led(ro_cfg_led_list_t **event_led);

void ro_cfg_default_event_led(ro_cfg_led_list_t **ret_event_led)
{
    ro_cfg_led_list_t *event_led;
    uint32_t len;
    uint32_t num;

    num = LED_EVENT_NUM;
    len = sizeof(struct ro_cfg_led) * num;
    event_led = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_led_list_t) + len);
    event_led->num = num;
    memcpy_s(event_led->leds, len, led_event, sizeof(led_event));
    *ret_event_led = event_led;
}
