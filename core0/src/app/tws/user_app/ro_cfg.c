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
#include "ro_cfg.h"
#include "assert.h"
#include "os_mem.h"
#include "string.h"
#include "modules.h"
#include "app_main.h"
#include "os_utils.h"
#include "data_dump.h"

#define RO_MSG_ID_DUMP_CFG1 1
#define RO_MSG_ID_DUMP_CFG2 2

static ro_cfg_t cfg;

static uint32_t ro_cfg_read(uint32_t id, void *buf, uint32_t *len)
{
    return storage_read(APP_BASE_ID, id, (uint32_t *)buf, len);
}

static void dump_configs_1(void)
{
    DBGLOG_RO_DBG("============== general cfg ==============\n");
    dump_bytes((uint8_t *)cfg.general, sizeof(*cfg.general));

    DBGLOG_RO_DBG("============== system cfg ==============\n");
    dump_bytes((uint8_t *)cfg.system, sizeof(*cfg.system));

    DBGLOG_RO_DBG("============== volume cfg ==============\n");
    dump_bytes((uint8_t *)cfg.volume, sizeof(*cfg.volume));

    DBGLOG_RO_DBG("============== battery cfg ==============\n");
    dump_bytes((uint8_t *)cfg.battery, sizeof(*cfg.battery));

    DBGLOG_RO_DBG("============== timeout cfg ==============\n");
    dump_bytes((uint8_t *)cfg.timeout, sizeof(*cfg.timeout));

    DBGLOG_RO_DBG("=========== tone_setting cfg ============\n");
    dump_bytes((uint8_t *)cfg.tone_setting, sizeof(*cfg.tone_setting));

    DBGLOG_RO_DBG("============== tone cfg ==============\n");
    dump_bytes((uint8_t *)cfg.tone,
                   sizeof(*cfg.tone) + cfg.tone->num * sizeof(cfg.tone->tones[0]));

    DBGLOG_RO_DBG("========== key_setting cfg ===========\n");
    if (cfg.key_setting) {
        dump_bytes((uint8_t *)cfg.key_setting, sizeof(*cfg.key_setting));
    }

    DBGLOG_RO_DBG("=========== key_event cfg ============\n");
    dump_bytes((uint8_t *)cfg.key_event,
                   sizeof(*cfg.key_event)
                       + cfg.key_event->num * sizeof(cfg.key_event->key_events[0]));
}

static void dump_configs_2(void)
{
    DBGLOG_RO_DBG("=========== event_led cfg ============\n");
    dump_bytes((uint8_t *)cfg.event_led,
                   sizeof(*cfg.event_led) + cfg.event_led->num * sizeof(cfg.event_led->leds[0]));

    DBGLOG_RO_DBG("=========== state_led cfg ============\n");
    dump_bytes((uint8_t *)cfg.state_led,
                   sizeof(*cfg.state_led) + cfg.state_led->num * sizeof(cfg.state_led->leds[0]));
}

static void ro_cfg_handle_msg(uint16_t msg_id, void *param)
{
    UNUSED(param);
    if (msg_id == RO_MSG_ID_DUMP_CFG1) {
        dump_configs_1();
    } else if (msg_id == RO_MSG_ID_DUMP_CFG2) {
        dump_configs_2();
    }
}

const ro_cfg_t *ro_cfg(void)
{
    return &cfg;
}

const ro_cfg_general_t *ro_gen_cfg(void)
{
    return cfg.general;
}

const ro_cfg_system_t *ro_sys_cfg(void)
{
    return cfg.system;
}

const ro_cfg_volume_t *ro_vol_cfg(void)
{
    return cfg.volume;
}

const ro_cfg_battery_t *ro_bat_cfg(void)
{
    return cfg.battery;
}

const ro_cfg_timeout_t *ro_to_cfg(void)
{
    return cfg.timeout;
}

const struct ro_cfg_features *ro_feat_cfg(void)
{
    return &cfg.system->features;
}

void ro_cfg_init(void)
{
    uint32_t ret;
    uint32_t num;
    uint32_t len;

    ro_cfg_general_t *general;
    ro_cfg_system_t *system;
    ro_cfg_volume_t *volume;
    ro_cfg_battery_t *battery;
    ro_cfg_timeout_t *timeout;
    ro_cfg_tone_setting_t *tone_setting;
    ro_cfg_tone_list_t *tone;
    ro_cfg_key_setting_t *key_setting;
    ro_cfg_key_event_list_t *key_event;
    ro_cfg_led_list_t *event_led;
    ro_cfg_led_list_t *state_led;

    general = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_general_t));
    assert(general);
    len = sizeof(ro_cfg_general_t);
    ret = ro_cfg_read(USER_GENERAL_ID, general, &len);
    if (ret || (len != sizeof(ro_cfg_general_t))) {
        DBGLOG_RO_ERR("read general error ret:%d len:%d\n", ret, len);
        void ro_cfg_default_general(ro_cfg_general_t * general);
        ro_cfg_default_general(general);
    }

    system = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_system_t));
    assert(system);
    len = sizeof(ro_cfg_system_t);
    ret = ro_cfg_read(USER_SYSTEM_ID, system, &len);
    if (ret || (len != sizeof(ro_cfg_system_t))) {
        DBGLOG_RO_ERR("read system error ret:%d len:%d\n", ret, len);
        void ro_cfg_default_system(ro_cfg_system_t * system);
        ro_cfg_default_system(system);
    }

    volume = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_volume_t));
    assert(volume);
    len = sizeof(ro_cfg_volume_t);
    ret = ro_cfg_read(USER_VOLUME_ID, volume, &len);
    if (ret || (len != sizeof(ro_cfg_volume_t))) {
        DBGLOG_RO_ERR("read volume error ret:%d len:%d\n", ret, len);
        void ro_cfg_default_volume(ro_cfg_volume_t * volume);
        ro_cfg_default_volume(volume);
    }

    battery = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_battery_t));
    assert(battery);
    len = sizeof(ro_cfg_battery_t);
    ret = ro_cfg_read(USER_BATTERY_ID, battery, &len);
    if (ret || (len != sizeof(ro_cfg_battery_t))) {
        DBGLOG_RO_ERR("read battery error ret:%d len:%d\n", ret, len);
        void ro_cfg_default_battery(ro_cfg_battery_t * battery);
        ro_cfg_default_battery(battery);
    }

    timeout = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_timeout_t));
    assert(timeout);
    len = sizeof(ro_cfg_timeout_t);
    ret = ro_cfg_read(USER_TIMER_ID, timeout, &len);
    if (ret || (len != sizeof(ro_cfg_timeout_t))) {
        DBGLOG_RO_ERR("read timeout error ret:%d len:%d\n", ret, len);
        void ro_cfg_default_timeout(ro_cfg_timeout_t * timeout);
        ro_cfg_default_timeout(timeout);
    }

    key_setting = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_key_setting_t));
    assert(key_setting);
    len = sizeof(ro_cfg_key_setting_t);
    ret = ro_cfg_read(USER_KEY_SETTING_ID, key_setting, &len);
    if (ret || (len != sizeof(ro_cfg_key_setting_t))) {
        DBGLOG_RO_ERR("read key_setting error ret:%d len:%d\n", ret, len);
        void ro_cfg_default_key_setting(ro_cfg_key_setting_t * key_setting);
        ro_cfg_default_key_setting(key_setting);
    }

    tone_setting = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_tone_setting_t));
    assert(tone_setting);
    len = sizeof(ro_cfg_tone_setting_t);
    ret = ro_cfg_read(USER_TONE_SETTING_ID, tone_setting, &len);
    if (ret || (len != sizeof(ro_cfg_tone_setting_t))) {
        DBGLOG_RO_ERR("read tone_setting error ret:%d len:%d\n", ret, len);
        memset(tone_setting, 0, sizeof(ro_cfg_tone_setting_t));
    }

    num = 0;
    len = sizeof(uint32_t);
    ret = ro_cfg_read(USER_TONE_NUM_ID, &num, &len);
    if (ret || (len != sizeof(uint32_t))) {
        DBGLOG_RO_ERR("read tone num error ret:%d len:%d\n", ret, len);
        tone = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_tone_list_t));
        assert(tone);
        tone->num = 0;
    } else {
        len = sizeof(struct ro_cfg_tone) * num;
        tone = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_tone_list_t) + len);
        assert(tone);
        if (num != 0) {
            ret = ro_cfg_read(USER_TONE_LIST_ID, tone->tones, &len);
            if (ret || (len != sizeof(struct ro_cfg_tone) * num)) {
                DBGLOG_RO_ERR("read tone list error ret:%d len:%d\n", ret, len);
                tone->num = 0;
            } else {
                tone->num = num;
            }
        } else {
            tone->num = 0;
        }
    }

    num = 0;
    len = sizeof(uint32_t);
    ret = ro_cfg_read(USER_EVENT_KEY_NUM_ID, &num, &len);
    if (ret || (len != sizeof(uint32_t))) {
        DBGLOG_RO_ERR("read key event num error ret:%d len:%d\n", ret, len);
#if 1
        void ro_cfg_default_key_event(ro_cfg_key_event_list_t * *key_event);
        ro_cfg_default_key_event(&key_event);
#else
        key_event = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_key_event_list_t));
        assert(key_event);
        key_event->num = 0;
#endif
    } else {
        len = sizeof(struct ro_cfg_key_event) * num;
        key_event = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_key_event_list_t) + len);
        assert(key_event);
        if (num != 0) {
            ret = ro_cfg_read(USER_EVENT_KEY_LIST_ID, key_event->key_events, &len);
            if (ret || (len != sizeof(struct ro_cfg_key_event) * num)) {
                DBGLOG_RO_ERR("read key event list error ret:%d len:%d\n", ret, len);
                key_event->num = 0;
            } else {
                key_event->num = num;
            }
        } else {
            key_event->num = 0;
        }
    }

    num = 0;
    len = sizeof(uint32_t);
    ret = ro_cfg_read(USER_EVENT_LED_NUM_ID, &num, &len);
    if (ret || (len != sizeof(uint32_t))) {
        DBGLOG_RO_ERR("read event led num error ret:%d len:%d\n", ret, len);
#if 1
        void ro_cfg_default_event_led(ro_cfg_led_list_t * *event_led);
        ro_cfg_default_event_led(&event_led);
#else
        event_led = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_led_list_t));
        assert(event_led);
        event_led->num = 0;
#endif
    } else {
        len = sizeof(struct ro_cfg_led) * num;
        event_led = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_led_list_t) + len);
        assert(event_led);
        if (num != 0) {
            ret = ro_cfg_read(USER_EVENT_LED_LIST_ID, event_led->leds, &len);
            if (ret || (len != sizeof(struct ro_cfg_led) * num)) {
                DBGLOG_RO_ERR("read event led list error ret:%d len:%d\n", ret, len);
                event_led->num = 0;
            } else {
                event_led->num = num;
            }
        } else {
            event_led->num = 0;
        }
    }

    num = 0;
    len = sizeof(uint32_t);
    ret = ro_cfg_read(USER_STATE_LED_NUM_ID, &num, &len);
    if (ret || (len != sizeof(uint32_t))) {
        DBGLOG_RO_ERR("read state led num error ret:%d len:%d\n", ret, len);
#if 1
        void ro_cfg_default_state_led(ro_cfg_led_list_t * *state_led);
        ro_cfg_default_state_led(&state_led);
#else
        state_led = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_led_list_t));
        assert(state_led);
        state_led->num = 0;
#endif
    } else {
        len = sizeof(struct ro_cfg_led) * num;
        state_led = os_mem_malloc(IOT_APP_MID, sizeof(ro_cfg_led_list_t) + len);
        assert(state_led);
        if (num != 0) {
            ret = ro_cfg_read(USER_STATE_LED_LIST_ID, state_led->leds, &len);
            if (ret || (len != sizeof(struct ro_cfg_led) * num)) {
                DBGLOG_RO_ERR("read state led list error ret:%d len:%d\n", ret, len);
                state_led->num = 0;
            } else {
                state_led->num = num;
            }
        } else {
            state_led->num = 0;
        }
    }

    cfg.general = general;
    cfg.system = system;
    cfg.volume = volume;
    cfg.battery = battery;
    cfg.timeout = timeout;
    cfg.tone_setting = tone_setting;
    cfg.tone = tone;
    cfg.key_setting = key_setting;
    cfg.key_event = key_event;
    cfg.event_led = event_led;
    cfg.state_led = state_led;

    app_register_msg_handler(MSG_TYPE_RO_CFG, ro_cfg_handle_msg);
    app_send_msg_delay(MSG_TYPE_RO_CFG, RO_MSG_ID_DUMP_CFG1, NULL, 0, 2000);
    app_send_msg_delay(MSG_TYPE_RO_CFG, RO_MSG_ID_DUMP_CFG2, NULL, 0, 3000);
}

void ro_cfg_deinit(void)
{
    if (cfg.general) {
        os_mem_free((void *)cfg.general);
        cfg.general = NULL;
    }
    if (cfg.system) {
        os_mem_free((void *)cfg.system);
        cfg.system = NULL;
    }
    if (cfg.volume) {
        os_mem_free((void *)cfg.volume);
        cfg.volume = NULL;
    }
    if (cfg.battery) {
        os_mem_free((void *)cfg.battery);
        cfg.battery = NULL;
    }
    if (cfg.battery) {
        os_mem_free((void *)cfg.battery);
        cfg.battery = NULL;
    }
    if (cfg.timeout) {
        os_mem_free((void *)cfg.timeout);
        cfg.timeout = NULL;
    }
    if (cfg.tone) {
        os_mem_free((void *)cfg.tone);
        cfg.tone = NULL;
    }
    if (cfg.key_setting) {
        os_mem_free((void *)cfg.key_setting);
        cfg.key_setting = NULL;
    }
    if (cfg.key_event) {
        os_mem_free((void *)cfg.key_event);
        cfg.key_event = NULL;
    }
    if (cfg.event_led) {
        os_mem_free((void *)cfg.event_led);
        cfg.event_led = NULL;
    }
    if (cfg.state_led) {
        os_mem_free((void *)cfg.state_led);
        cfg.state_led = NULL;
    }
}

void ro_cfg_release_key_setting(void)
{
    if (cfg.key_setting) {
        os_mem_free((void *)cfg.key_setting);
        cfg.key_setting = NULL;
    }
}
