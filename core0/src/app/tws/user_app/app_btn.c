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
#include "string.h"
#include "os_mem.h"
#include "key_sensor.h"
#include "ro_cfg.h"
#include "usr_cfg.h"
#include "app_main.h"
#include "app_btn.h"
#include "app_pm.h"
#include "app_led.h"
#include "app_bt.h"
#include "app_evt.h"
#include "app_wws.h"
#include "app_econn.h"

#define APP_BTN_MSG_ID_KEY_CALLBACK 1
#define APP_BTN_MSG_ID_KEY_OPEN     2

#ifndef MAX_CUS_KEY_EVENT
#define MAX_CUS_KEY_EVENT (8)
#endif

#ifndef APP_BTN_DEBUG
#define APP_BTN_DEBUG 0
#endif

typedef struct {
    uint8_t id;
    uint8_t src;
    key_pressed_type_t type : 8;
    /* user event id */
    uint16_t event;
    /* bt system state bit mask */
    uint16_t sys_state;
} cus_key_entry_t;

typedef struct {
    /* set if cfg modified after load from flash */
    bool_t dirty;
    /* Num of user key event */
    uint32_t num;
    /* List of user key event */
    cus_key_entry_t key_events[MAX_CUS_KEY_EVENT];
} cus_key_tbl_t;

static cus_key_tbl_t the_cus_key = {0};
static cus_key_tbl_t *cus_key = &the_cus_key;

static bool_t has_btn_power_on = false;
static key_pressed_type_t last_pressed_type = BTN_LAST;

static uint16_t app_btn_cus_evt_hdl(key_pressed_info_t *info)
{
    uint32_t i;

    //only check the first

    for (i = 0; i < cus_key->num; i++) {
        if ((((cus_key->key_events[i].id == info->id[0])
              && (cus_key->key_events[i].src == info->src[0]))
             || (cus_key->key_events[i].id == 0xFF))
            && (cus_key->key_events[i].type == info->type[0])
            && (app_bt_get_sys_state() & cus_key->key_events[i].sys_state)) {
            last_pressed_type = info->type[0];
            return cus_key->key_events[i].event;
        }
    }

    return 0;
}

static bool is_same_key_pressed(const struct ro_cfg_key_press_info *key1,
                                const key_pressed_info_t *key2)
{
    uint32_t i, j;

    assert(key1);
    assert(key2);

    if (key1->pressed_num != key2->num)
        return false;

    for (i = 0; i < key1->pressed_num; i++) {
        for (j = 0; j < key2->num; j++) {
            if ((key1->keys[i].id == key2->id[j]) && (key1->keys[i].type == key2->type[j])
                && (key1->keys[i].src == key2->src[j])) {
                break;
            }
        }

        if (j == key2->num) {
            return false;
        }
    }

    return true;
}

/*   K E Y P R E S S E D _ P A R S E   */
/*-------------------------------------------------------------------------

-------------------------------------------------------------------------*/
static void keypressed_parse(key_pressed_info_t *key_press_info)
{
    uint16_t event;
    const ro_cfg_key_event_list_t *key_event;

    if (app_econn_handle_key_pressed(key_press_info)) {
        DBGLOG_EVT_DBG("keypressed_parse num:%d type[0]:%d handled by econn\n", key_press_info->num,
                       key_press_info->type[0]);
        return;
    }

    key_event = ro_cfg()->key_event;
    assert(key_press_info != NULL);
    assert(key_event != NULL);

    DBGLOG_BTN_DBG("keypressed_parse: key[0]={id:%d,src:%d,type:%d} pressed_num:%d state:0x%X\n",
                   key_press_info->id[0], key_press_info->src[0], key_press_info->type[0],
                   key_press_info->num, app_bt_get_sys_state());

    if (key_press_info->num == 1) {
        if (key_press_info->type[0] == BTN_TYPE_PRESS) {
            app_evt_send(EVTSYS_BUTTON_PRESS);
        } else if (key_press_info->type[0] == BTN_TYPE_RELEASE) {
            app_evt_send(EVTSYS_BUTTON_RELEASE);
        } else if (key_press_info->type[0] == BTN_TYPE_SINGLE) {
            app_evt_send(EVTSYS_BUTTON_SINGLE);
        } else if (key_press_info->type[0] == BTN_TYPE_DOUBLE) {
            app_evt_send(EVTSYS_BUTTON_DOUBLE);
        } else if (key_press_info->type[0] == BTN_TYPE_LONG) {
            app_evt_send(EVTSYS_BUTTON_LONG);
        } else if (key_press_info->type[0] == BTN_TYPE_VLONG) {
            app_evt_send(EVTSYS_BUTTON_VLONG);
        }
    }

    /* 1. customized user event has higher priority. */
    if ((cus_key->num > 0) && ((event = app_btn_cus_evt_hdl(key_press_info)) != 0)) {
        app_evt_send(event);
        DBGLOG_BTN_DBG("keypressed_parse: hit cus event %d\n", event);
        return;
    }

    /* 2. no customized user evt hit, then check tools config user event. */
    for (uint32_t i = 0; i < key_event->num; i++) {
        if ((is_same_key_pressed(&key_event->key_events[i].pressed_info, key_press_info))
            && (app_bt_get_sys_state() & key_event->key_events[i].system_state)) {
            event = key_event->key_events[i].event;
            last_pressed_type = key_press_info->type[0];
            app_evt_send(event);
            return;
        }
    }
    return;
}

static void btn_callback(const key_pressed_info_t *info)
{
    app_send_msg(MSG_TYPE_BTN, APP_BTN_MSG_ID_KEY_CALLBACK, info, sizeof(key_pressed_info_t));
}

static void app_btn_get_key_id(key_id_cfg_t *p_key_cfg, const ro_cfg_key_event_list_t *p_event_key)
{
    uint32_t i, j, k;
    uint8_t key_id = 0;
    uint8_t key_src = 0;

    assert(p_key_cfg != NULL);
    assert(p_event_key != NULL);

    for (i = 0; i < p_event_key->num; i++) {
        assert(p_event_key->key_events[i].pressed_info.pressed_num <= MAX_KEY_NUM);
        for (j = 0; j < p_event_key->key_events[i].pressed_info.pressed_num; j++) {

            key_id = p_event_key->key_events[i].pressed_info.keys[j].id;
            key_src = p_event_key->key_events[i].pressed_info.keys[j].src;

            for (k = 0; k < p_key_cfg->num; k++) {
                if ((p_key_cfg->id[k] == key_id) && (p_key_cfg->src[k] == key_src)) {
                    break;
                }
            }

            if (k == p_key_cfg->num) {
                if (p_key_cfg->num >= MAX_KEY_NUM) {
                    DBGLOG_BTN_DBG("btn num: %d, overflow\n", p_key_cfg->num);
                    return;
                }
                p_key_cfg->num++;
                p_key_cfg->id[k] = key_id;
                p_key_cfg->src[k] = key_src;

                DBGLOG_BTN_DBG("btn %d: key_id %d, key_src %d\n", k, key_id, key_src);
            }

            if (p_event_key->key_events[i].event == EVTUSR_POWER_ON) {
                has_btn_power_on = true;
            }
        }
    }
}

static void app_btn_cus_key_load(void)
{
    uint32_t len;
    uint32_t ret;

    len = sizeof(cus_key_tbl_t);
    ret = storage_read(APP_BASE_ID, APP_CUS_EVT_KEY_ID, (uint32_t *)cus_key, &len);

    if ((len != sizeof(cus_key_tbl_t)) || (ret != RET_OK)) {
        DBGLOG_BTN_ERR("app_btn_cus_key_load error len:%d ret:%d\n", len, ret);
        memset(cus_key, 0, sizeof(cus_key_tbl_t));
    } else {
        DBGLOG_BTN_DBG("app_btn_cus_key_load success cus_key num: %d\n", cus_key->num);
    }
}

void app_btn_open_sensor(void)
{
    key_cfg_t key_cfg;

    const ro_cfg_key_setting_t *key_setting;
    const ro_cfg_key_event_list_t *key_event;

    key_setting = ro_cfg()->key_setting;
    key_event = ro_cfg()->key_event;

    memset(&key_cfg, 0x00, sizeof(key_cfg));

#if APP_BTN_DEBUG
    DBGLOG_BTN_DBG("long_time: %d\n", key_setting->long_time);
    DBGLOG_BTN_DBG("vlong_time : %d\n", key_setting->vlong_time);
    DBGLOG_BTN_DBG("vvlong_time: %d\n", key_setting->vvlong_time);
    DBGLOG_BTN_DBG("repeat_start_time: %d\n", key_setting->repeat_start_time);
    DBGLOG_BTN_DBG("repeat_rate: %d\n", key_setting->repeat_rate);
    DBGLOG_BTN_DBG("max_multi_tap_interval: %d\n", key_setting->max_multi_tap_interval);
    DBGLOG_BTN_DBG("debounce_check_interval: %d\n", key_setting->debounce_check_interval);

    for (uint32_t i = 0; i < key_event->num; i++) {

        DBGLOG_BTN_DBG("event %d, state 0x%x,", key_event->key_events[i].event,
                       key_event->key_events[i].system_state);

        for (uint32_t j = 0; j < key_event->key_events[i].pressed_info.pressed_num; j++) {
            DBGLOG_BTN_DBG(" key_id %d, src %d, type %d",
                           key_event->key_events[i].pressed_info.keys[j].id,
                           key_event->key_events[i].pressed_info.keys[j].src,
                           key_event->key_events[i].pressed_info.keys[j].type);
        }
        DBGLOG_BTN_DBG("\n");
    }
#endif

    /* form ID cfg */
    app_btn_get_key_id(&key_cfg.id, key_event);

    /* form time cfg */
    key_cfg.time.long_time = key_setting->long_time;
    key_cfg.time.vlong_time = key_setting->vlong_time;
    key_cfg.time.vvlong_time = key_setting->vvlong_time;
    key_cfg.time.start_time = key_setting->repeat_start_time;
    key_cfg.time.repeat_time = key_setting->repeat_rate;
    key_cfg.time.multi_tap_interval = key_setting->max_multi_tap_interval
        + (((uint16_t)(key_setting->max_multi_tap_interval_high)) << 8);
    key_cfg.time.debounce_time = key_setting->debounce_check_interval;

    /* form threshold cfg */
    key_cfg.thres.climb_thres = key_setting->climb_thres;
    key_cfg.thres.fall_thres = key_setting->fall_thres;

    key_sensor_open(&key_cfg, has_btn_power_on);
    return;
}

int app_btn_cus_key_add_with_id(uint8_t id, uint8_t src, key_pressed_type_t type,
                                uint16_t state_mask, uint16_t event)
{
    cus_key_entry_t *key_evt;
    bool_t found = false;

    for (uint32_t i = 0; i < cus_key->num; i++) {
        key_evt = &cus_key->key_events[i];
        if ((key_evt->id == id) && (key_evt->src == src) && (key_evt->type == type)
            && (key_evt->sys_state == state_mask)) {

            /* same entry, update it. */
            DBGLOG_BTN_DBG(
                "app_btn_cus_key_add_with_id same cfg, type %d, old_evt %d, new_evt %d\n", type,
                key_evt->event, event);
            key_evt->event = event;
            found = true;
            break;
        }
    }

    if (!found) {
        /* new entry. */
        if (cus_key->num >= MAX_CUS_KEY_EVENT) {
            DBGLOG_BTN_ERR("app_btn_cus_key_add_with_id failed, table full\n");
            return RET_NOMEM;
        }
        /* insert it. */
        key_evt = &cus_key->key_events[cus_key->num];
        key_evt->id = id;
        key_evt->src = src;
        key_evt->type = type;
        key_evt->sys_state = state_mask;
        key_evt->event = event;
        cus_key->num++;

        DBGLOG_BTN_DBG("app_btn_cus_key_add_with_id new entry, type %d, evt %d.\n", type, event);
    }

    cus_key->dirty = true;
    return RET_OK;
}

int app_btn_cus_key_add(key_pressed_type_t type, uint16_t state_mask, uint16_t event)
{
    return app_btn_cus_key_add_with_id(0xFF, 0xFF, type, state_mask, event);
}

uint16_t app_btn_cus_key_read_with_id(uint8_t id, uint8_t src, key_pressed_type_t type,
                                      uint16_t state_mask)
{
    cus_key_entry_t *key_evt;

    for (uint32_t i = 0; i < cus_key->num; i++) {
        key_evt = &cus_key->key_events[i];
        if ((key_evt->id == id) && (key_evt->src == src) && (key_evt->type == type)
            && (key_evt->sys_state == state_mask)) {

            /* found entry. */
            DBGLOG_BTN_DBG("app_btn_cus_key_read_with_id, found event %d.\n", key_evt->event);
            return key_evt->event;
        }
    }
    return 0;
}

uint16_t app_btn_cus_key_read(key_pressed_type_t type, uint16_t state_mask)
{
    return app_btn_cus_key_read_with_id(0xFF, 0xFF, type, state_mask);
}

int app_btn_cus_key_reset(void)
{
    memset((void *)cus_key, 0x00, sizeof(cus_key_tbl_t));
    cus_key->dirty = true;

    DBGLOG_BTN_DBG("app_btn_cus_key_reset, all cleared. \n");
    return RET_OK;
}

static void app_btn_cus_key_write_flash(void)
{
    uint32_t ret;

    if (!cus_key->dirty) {
        return;
    }

    cus_key->dirty = false;
    ret =
        storage_write(APP_BASE_ID, APP_CUS_EVT_KEY_ID, (uint32_t *)cus_key, sizeof(cus_key_tbl_t));
    DBGLOG_BTN_DBG("app_btn_cus_key_write_flash ret:%d\n", ret);
}

static void app_btn_handle_msg(uint16_t msg_id, void *param)
{
    switch (msg_id) {
        case APP_BTN_MSG_ID_KEY_CALLBACK:
            keypressed_parse((key_pressed_info_t *)param);
            break;
        case APP_BTN_MSG_ID_KEY_OPEN:
            app_btn_open_sensor();
            break;

        default:
            break;
    }
}

bool_t app_btn_all_released(void)
{
    return key_sensor_all_key_released();
}

void app_btn_init(void)
{
    key_id_cfg_t key_id;
    const ro_cfg_key_event_list_t *key_event;

    DBGLOG_BTN_DBG("app_btn_init\n");
    app_register_msg_handler(MSG_TYPE_BTN, app_btn_handle_msg);

    memset(&key_id, 0x00, sizeof(key_id));
    key_event = ro_cfg()->key_event;
    app_btn_get_key_id(&key_id, key_event);

    key_sensor_init(&key_id, btn_callback);
    app_btn_cus_key_load();

    app_send_msg(MSG_TYPE_BTN, APP_BTN_MSG_ID_KEY_OPEN, NULL, 0);
}

void app_btn_deinit(void)
{
    app_btn_cus_key_write_flash();

    key_sensor_deinit(has_btn_power_on);
}

void app_btn_send_virtual_btn(uint8_t key_id, uint8_t key_src, key_pressed_type_t key_type)
{
    key_pressed_info_t info;
    info.num = 1;
    info.id[0] = key_id;
    info.src[0] = key_src;
    info.type[0] = key_type;
    app_send_msg(MSG_TYPE_BTN, APP_BTN_MSG_ID_KEY_CALLBACK, &info, sizeof(key_pressed_info_t));
}

bool_t app_btn_has_btn_power_on(void)
{
    return has_btn_power_on;
}

key_pressed_type_t app_btn_get_last_triggered_type(void)
{
    return last_pressed_type;
}
