#include "key_sensor.h"

#include "types.h"
#include "iot_rtc.h"
#include "string.h"
#include "key_base.h"
#include "vendor_msg.h"

#define KEY_BASE_DEBUG             0
#define KEY_BASE_SELF_RELEASE_TIME 20000 /* 20s */
#define KEY_BASE_CLK_DIFF(now, prev) \
    ((now) > (prev)) ? ((now) - (prev)) : (0xFFFFFFFF - (prev) + (now) + 1)

typedef enum {
    BTN_STATE_INIT = 0,
    BTN_STATE_PRESS,
    BTN_STATE_WAIT_RELEASE,
    BTN_STATE_RELEASE,
} BTN_STATE;

typedef enum {
    DECT_INIT = 0,
    DECT_PROGRESSING,
    DECT_DONE,
} COMBINED_KEY_DECT_STATE;

typedef struct {
    /* the IO ID bind to this key. it will be the gpio_id for simple io and
       pad_id for internal touch. */
    uint16_t io_id;
    uint8_t key_src;
    /* key port configed in config tool.valid value is 0~2 for now */
    uint8_t key_id;
    uint32_t pstate;
    uint32_t tap_count;
    uint32_t repeat_time;
    uint32_t press_time;
    uint32_t release_time;
    uint32_t hold_timeout;
} KeyState_t;

static struct {
    uint32_t num;
    KeyState_t key[MAX_KEY_NUM];
} Keys;

typedef struct {
    uint8_t index[MAX_KEY_NUM];
    uint32_t type[MAX_KEY_NUM];
} combined_key_cache_t;

static uint32_t g_key_mask = 0;
static COMBINED_KEY_DECT_STATE g_combined_key_state = DECT_INIT;
static combined_key_cache_t g_key_cache;
static key_time_cfg_t g_key_base_time;
static key_callback_t g_key_base_callback;
static bool_t is_shutdown;

const char *const button_type_name[] = {
    "BTN_TYPE_SINGLE",           //
    "BTN_TYPE_DOUBLE",           //
    "BTN_TYPE_TRIPLE",           //
    "BTN_TYPE_QUADRUPLE",        //
    "BTN_TYPE_QUINTUPLE",        //
    "BTN_TYPE_SEXTUPLE",         //
    "BTN_TYPE_SHORT",            //
    "BTN_TYPE_LONG",             //
    "BTN_TYPE_LONG_RELEASE",     //
    "BTN_TYPE_VLONG",            //
    "BTN_TYPE_VLONG_RELEASE",    //
    "BTN_TYPE_VVLONG",           //
    "BTN_TYPE_VVLONG_RELEASE",   //
    "BTN_TYPE_REPEAT",           //
    "BTN_TYPE_PRESS",            //
    "BTN_TYPE_RELEASE",          //
    "BTN_TYPE_UNKNOW"            //
};

const char *const combined_key_detect_state[] = {
    "DECT_INIT",          //
    "DECT_PROGRESSING",   //
    "DECT_DONE"           //
};

static uint32_t get_key_type_xlong(uint8_t ki, uint32_t now_time)
{
    uint32_t hold_time;
    uint32_t btn_type = BTN_LAST;

    hold_time = KEY_BASE_CLK_DIFF(now_time, Keys.key[ki].press_time);

    if (hold_time >= Keys.key[ki].hold_timeout) {
        if (hold_time >= g_key_base_time.vvlong_time) {
            btn_type = BTN_TYPE_VVLONG;
        } else if (hold_time >= g_key_base_time.vlong_time) {
            Keys.key[ki].hold_timeout = g_key_base_time.vvlong_time;
            btn_type = BTN_TYPE_VLONG;
        } else if (hold_time >= g_key_base_time.long_time) {
            Keys.key[ki].hold_timeout = g_key_base_time.vlong_time;
            btn_type = BTN_TYPE_LONG;
        }
    }
    return btn_type;
}

static uint32_t get_key_type_repeat(uint8_t ki, uint32_t now_time)
{
    uint32_t hold_time, btn_type = BTN_LAST;

    hold_time = KEY_BASE_CLK_DIFF(now_time, Keys.key[ki].press_time);

    if (hold_time > g_key_base_time.start_time) {
        if (Keys.key[ki].repeat_time == 0) {
            Keys.key[ki].repeat_time = now_time;
        } else if (now_time - Keys.key[ki].repeat_time >= g_key_base_time.repeat_time) {
            Keys.key[ki].repeat_time = now_time;
            btn_type = BTN_TYPE_REPEAT;
        }
    }
    return btn_type;
}

static bool is_key_pressed(uint8_t ki)
{
    return (g_key_mask & (1 << ki)) ? true : false;
}

static bool is_all_key_init_state(void)
{
    uint32_t i;

    for (i = 0; i < MAX_KEY_NUM; i++) {
        if (Keys.key[i].pstate != BTN_STATE_INIT) {
            return false;
        }
    }
    return true;
}

static bool is_combined_key_type(uint32_t btn_type)
{
    if ((btn_type == BTN_TYPE_SHORT) || (btn_type == BTN_TYPE_LONG) || (btn_type == BTN_TYPE_VLONG)
        || (btn_type == BTN_TYPE_VVLONG)) {
        return true;
    }

    return false;
}

static void update_press_time(uint32_t now_time)
{
    uint32_t i;

    for (i = 0; i < Keys.num; i++) {
        if (Keys.key[i].pstate != BTN_STATE_INIT) {
            Keys.key[i].press_time = now_time;
            Keys.key[i].hold_timeout = g_key_base_time.long_time;
        }
    }
}

static uint16_t get_key_index_via_io_id(uint16_t io_id)
{
    uint32_t i;

    for (i = 0; i < Keys.num; i++) {
        if (Keys.key[i].io_id == io_id) {
            return i;
        }
    }

    return INVALID_UCHAR;
}

static const char *get_btn_type_name(uint32_t btn_type)
{
    uint32_t i;

    for (i = 0; i < (sizeof(button_type_name) / sizeof(button_type_name[0])); i++) {
        if (btn_type == i) {
            break;
        }
    }

    return button_type_name[i];
}

static const char *get_combined_key_state_name(uint32_t state)
{
    uint32_t i;

    for (i = 0; i < (sizeof(combined_key_detect_state) / sizeof(combined_key_detect_state[0]));
         i++) {
        if (state == i) {
            break;
        }
    }

    return combined_key_detect_state[i];
}

static uint8_t get_pressed_key_num(void)
{
    uint32_t i, key_mask;
    uint8_t num = 0;

    key_mask = g_key_mask;

    for (i = 0; (i < Keys.num) && (key_mask != 0); i++) {

        if (key_mask & (1 << i)) {

            num++;
            key_mask &= ~(1 << i);
        }
    }
    return num;
}

static uint8_t get_pressed_key_index(uint8_t ki[], uint8_t len)
{
    uint32_t i, key_mask;
    uint8_t num = 0;

    key_mask = g_key_mask;

    for (i = 0; (i < Keys.num) && (key_mask != 0) && (len > num); i++) {
        if (key_mask & (1 << i)) {

            key_mask &= ~(1 << i);
            ki[num] = i;
            num++;
        }
    }
    return num;
}

#if KEY_BASE_DEBUG
static uint16_t get_gpio_via_key_index(uint8_t ki)
{
    uint32_t i;

    if (ki < Keys.num) {
        return Keys.key[ki].io_id;
    } else {
        return INVALID_UCHAR;
    }
}

static bool is_multi_tap_type(uint32_t btn_type)
{
    if ((btn_type == BTN_TYPE_SINGLE) || (btn_type == BTN_TYPE_DOUBLE)
        || (btn_type == BTN_TYPE_TRIPLE) || (btn_type == BTN_TYPE_QUADRUPLE)
        || (btn_type == BTN_TYPE_QUINTUPLE) || (btn_type == BTN_TYPE_SEXTUPLE)) {
        return true;
    }

    return false;
}

static bool is_all_multi_tap_checking_done(void)
{
    uint32_t i;

    for (i = 0; i < MAX_KEY_NUM; i++) {
        if (Keys.key[i].release_time != 0) {
            return false;
        }
    }
    return true;
}
#endif

static void send_key_type(uint32_t key_type, uint8_t ki)
{
    key_pressed_info_t key_press_info;

    if ((key_type == BTN_LAST) || (g_combined_key_state != DECT_INIT) || (is_shutdown == true)) {
        /* is shuting down or not a valid button type triggered.  */
        return;
    }

    DBGLOG_KEY_SENSOR_RAW("send_key_type: key_type:%s key_id:%d key_src:%d\n",
                          get_btn_type_name(key_type), Keys.key[ki].key_id, Keys.key[ki].key_src);

    key_press_info.num = 1;
    key_press_info.id[0] = Keys.key[ki].key_id;
    key_press_info.src[0] = Keys.key[ki].key_src;
    key_press_info.type[0] = key_type;

    if (g_key_base_callback) {
        g_key_base_callback(&key_press_info);
    }
    return;
}

static key_check_result_t get_key_type_multi_tap(uint8_t ki, uint32_t now_time)
{
    uint32_t release_last, btn_type = BTN_LAST;
    key_check_result_t ret = CHECK_PROGRESSING;

    release_last = KEY_BASE_CLK_DIFF(now_time, Keys.key[ki].release_time);

    if ((Keys.key[ki].release_time != 0) && (release_last >= g_key_base_time.multi_tap_interval)) {

        /* 1. timeout, determine multi-tap type */
        DBGLOG_KEY_SENSOR_INFO("multi-tap type: key_id:%d tap_count:%d released:%d interval %d\n",
                               Keys.key[ki].key_id, Keys.key[ki].tap_count, release_last,
                               g_key_base_time.multi_tap_interval);

        if (Keys.key[ki].tap_count == 1)
            btn_type = BTN_TYPE_SINGLE;
        else if (Keys.key[ki].tap_count == 2)
            btn_type = BTN_TYPE_DOUBLE;
        else if (Keys.key[ki].tap_count == 3)
            btn_type = BTN_TYPE_TRIPLE;
        else if (Keys.key[ki].tap_count == 4)
            btn_type = BTN_TYPE_QUADRUPLE;
        else if (Keys.key[ki].tap_count == 5)
            btn_type = BTN_TYPE_QUINTUPLE;
        else if (Keys.key[ki].tap_count == 6)
            btn_type = BTN_TYPE_SEXTUPLE;

        if (btn_type != BTN_LAST) {
            DBGLOG_KEY_SENSOR_RAW("key_id %d get %s on %u.\n", Keys.key[ki].key_id,
                                  get_btn_type_name(btn_type), now_time);
        }

        /* 2. clean tap_count and release_time, set key state to INIT. */
        Keys.key[ki].tap_count = 0;
        Keys.key[ki].release_time = 0;
        Keys.key[ki].pstate = BTN_STATE_INIT;

        /* 3. send key type. */
        send_key_type(btn_type, ki);

        /* 4. stop timer and set combined-key state to DECT_INIT if all keys go
              back to INIT state. */
        if (is_all_key_init_state()) {
            g_combined_key_state = DECT_INIT;
            ret = CHECK_DONE;
            DBGLOG_KEY_SENSOR_INFO("all keys return to INIT state.\n");
        }
    }

    return ret;
}

static void send_combined_key_type(uint32_t key_type, uint8_t key_num, uint8_t ki[])
{
    key_pressed_info_t key_press_info;
    uint32_t i;
    uint8_t index;

    if ((key_type == BTN_LAST) || (is_shutdown == true)) {
        /* is shuting down or not a valid button type triggered.  */
        return;
    }

    DBGLOG_KEY_SENSOR_RAW("send_combined_key_type: key_type:%s key_id: ",
                          get_btn_type_name(key_type));
    for (i = 0; i < key_num; i++) {
        index = ki[i];
        key_press_info.id[i] = Keys.key[index].key_id;
        key_press_info.src[i] = Keys.key[index].key_src;
        key_press_info.type[i] = key_type;

        DBGLOG_KEY_SENSOR_INFO("%d ", Keys.key[index].key_id);
    }
    DBGLOG_KEY_SENSOR_INFO("\n");

    key_press_info.num = key_num;
    if (g_key_base_callback) {
        g_key_base_callback(&key_press_info);
    }
}

void key_base_pressed(uint16_t io_id, uint32_t time_offset)
{
    uint8_t ki;
    uint32_t now_time, rtc_time;

    ki = get_key_index_via_io_id(io_id);
    if ((ki == INVALID_UCHAR) || is_key_pressed(ki)) {
        DBGLOG_KEY_SENSOR_ERROR("(ki %d, io %d) has been pressed, key_mask:%d\n", ki, io_id,
                                g_key_mask);
        return;
    }

    rtc_time = iot_rtc_get_global_time_ms();
    now_time = rtc_time - time_offset;

    DBGLOG_KEY_SENSOR_RAW("btn pressed on %u id:%d io:%d mask:0x%02x offset:%d cmb:%s\n", rtc_time,
                          Keys.key[ki].key_id, io_id, g_key_mask, time_offset,
                          get_combined_key_state_name(g_combined_key_state));

    /* 1. run combined key state machine */
    switch (g_combined_key_state) {
        case DECT_INIT:
            send_key_type(BTN_TYPE_PRESS, ki);
            if (get_pressed_key_num() != 0) {
                update_press_time(now_time);
                g_combined_key_state = DECT_PROGRESSING;
                DBGLOG_KEY_SENSOR_INFO("combined key enter DECT_PROGRESSING. \n");
            }
            break;

        case DECT_PROGRESSING:
            update_press_time(now_time);
            break;

        case DECT_DONE: {
            DBGLOG_KEY_SENSOR_INFO("combined key detecting done, ignore press. \n");
            return;
        }
    }

    /* 2. set key_mask and other times, set single key pstate. */
    g_key_mask |= (1 << ki);
    Keys.key[ki].press_time = now_time;
    Keys.key[ki].hold_timeout = g_key_base_time.long_time;
    Keys.key[ki].release_time = 0;
    Keys.key[ki].pstate = BTN_STATE_PRESS;
}

void key_base_released(uint16_t io_id)
{
    uint8_t pressed_key_num;
    uint8_t ki;
    uint32_t now_time, hold_time, btn_type = BTN_LAST;

    now_time = iot_rtc_get_global_time_ms();
    pressed_key_num = get_pressed_key_index(g_key_cache.index, MAX_KEY_NUM);

    ki = get_key_index_via_io_id(io_id);
    if ((ki != INVALID_UCHAR) && is_key_pressed(ki)) {
        g_key_mask &= ~(1 << ki);
    } else {
        DBGLOG_KEY_SENSOR_ERROR("io %d not pressed. \n", io_id);
        return;
    }

    /* 1. check hold_time to determine xLong release type. */
    hold_time = KEY_BASE_CLK_DIFF(now_time, Keys.key[ki].press_time);

    if (hold_time >= g_key_base_time.vvlong_time)
        btn_type = BTN_TYPE_VVLONG_RELEASE;
    else if (hold_time >= g_key_base_time.vlong_time)
        btn_type = BTN_TYPE_VLONG_RELEASE;
    else if (hold_time >= g_key_base_time.long_time)
        btn_type = BTN_TYPE_LONG_RELEASE;
    else if (hold_time < g_key_base_time.long_time)
        btn_type = BTN_TYPE_SHORT;

    DBGLOG_KEY_SENSOR_INFO("btn released on %u key:%d io:%d hold:%d tap:%d mask:0x%x\n", now_time,
                           Keys.key[ki].key_id, io_id, hold_time, Keys.key[ki].tap_count,
                           g_key_mask);

    /* 2. send single-key type and combined-key type. */
    switch (g_combined_key_state) {
        case DECT_INIT:
            send_key_type(BTN_TYPE_RELEASE, ki);
            send_key_type(btn_type, ki);
            break;
        case DECT_PROGRESSING:
            g_combined_key_state = DECT_DONE;
            DBGLOG_KEY_SENSOR_INFO("combined key enter DECT_DONE. \n");
            if (is_combined_key_type(btn_type)) {
                send_combined_key_type(btn_type, pressed_key_num, g_key_cache.index);
            }
            break;
        case DECT_DONE:
            DBGLOG_KEY_SENSOR_INFO("combined key detecting done, ignore release. \n");
            break;
    }

    /* 3. accumulate tap_count, multi-tap only counts short type. */
    if (btn_type == BTN_TYPE_SHORT) {
        Keys.key[ki].tap_count++;
    }
    Keys.key[ki].release_time = now_time;

    /* 4. do some cleanning. */
    Keys.key[ki].repeat_time = 0;
    Keys.key[ki].press_time = 0;
    Keys.key[ki].pstate = BTN_STATE_RELEASE;
}

key_check_result_t key_base_check_type(void)
{
    uint32_t i, btn_type, now_time;
    key_check_result_t ret = CHECK_PROGRESSING;
    uint8_t combined_key_num = 0;

    /* polling each port */
    now_time = iot_rtc_get_global_time_ms();
    for (i = 0; i < Keys.num; i++) {

        switch (Keys.key[i].pstate) {
            case BTN_STATE_INIT:
                break;

            case BTN_STATE_PRESS:
                /* get xlong type.  */
                if ((btn_type = get_key_type_xlong(i, now_time)) != BTN_LAST) {

                    DBGLOG_KEY_SENSOR_RAW("key_id %d get %s on %u.\n", Keys.key[i].key_id,
                                          get_btn_type_name(btn_type), now_time);

                    if (btn_type == BTN_TYPE_VVLONG)
                        Keys.key[i].pstate = BTN_STATE_WAIT_RELEASE;

                    send_key_type(btn_type, i);
                    if (g_combined_key_state == DECT_PROGRESSING) {
                        g_key_cache.type[combined_key_num] = btn_type;
                        g_key_cache.index[combined_key_num] = i;
                        combined_key_num++;
                    }

                    /* don't report multi tap if users tap single+long. */
                    Keys.key[i].tap_count = 0;
                }
            /* fall through */
            case BTN_STATE_WAIT_RELEASE:
                /* get repeat type.  */
                if ((btn_type = get_key_type_repeat(i, now_time)) != BTN_LAST) {
                    DBGLOG_KEY_SENSOR_RAW("key_id %d get %s on %u.\n", Keys.key[i].key_id,
                                          get_btn_type_name(btn_type), now_time);
                    send_key_type(btn_type, i);
                }
                break;

            case BTN_STATE_RELEASE:
                /* get multi-tap type.  */
                ret = get_key_type_multi_tap(i, now_time);
                break;
        }
    }

    if (combined_key_num != 0) {

        DBGLOG_KEY_SENSOR_INFO("combined_key_num %d, pressed key num %d, key_mask 0x%x.\n",
                               combined_key_num, get_pressed_key_num(), g_key_mask);
        send_combined_key_type(g_key_cache.type[0], combined_key_num, g_key_cache.index);
    }

    return ret;
}

bool_t key_base_is_key_pressed(uint8_t key_id, uint8_t key_src)
{
    uint32_t i;

    for (i = 0; i < Keys.num; i++) {
        if ((Keys.key[i].key_id == key_id) && (Keys.key[i].key_src == key_src)) {
            return is_key_pressed(i);
        }
    }

    DBGLOG_KEY_SENSOR_ERROR("the (io_id %d, key_src %d) is not registered.\n", key_id, key_src);
    return false;
}

bool_t key_base_all_key_released(void)
{
    return (g_key_mask == 0) ? true : false;
}

void key_base_init(key_callback_t callback)
{
    g_key_base_callback = callback;
    is_shutdown = false;
}

void key_base_deinit()
{
    is_shutdown = true;
}

void key_base_register(uint8_t key_id, uint8_t key_src, uint16_t io_id)
{
    uint32_t i;

    for (i = 0; i < Keys.num; i++) {
        if ((Keys.key[i].key_id == key_id) && (Keys.key[i].key_src == key_src)) {
            Keys.key[i].io_id = io_id;
            break;
        }
    }

    if ((i == Keys.num) && (i < MAX_KEY_NUM)) {
        Keys.key[i].io_id = io_id;
        Keys.key[i].key_id = key_id;
        Keys.key[i].key_src = key_src;
        Keys.key[i].pstate = BTN_STATE_INIT;
        Keys.num++;
    }

    DBGLOG_KEY_SENSOR_INFO("key_base_register Keys.num = %d. \n", Keys.num);
}

void key_base_set_time_cfg(const key_time_cfg_t *time_cfg)
{
    if (time_cfg == NULL) {
        DBGLOG_KEY_SENSOR_ERROR("key_base_set_time_cfg illegal, time_cfg = null\n");
    }
    memcpy(&g_key_base_time, time_cfg, sizeof(key_time_cfg_t));
}
