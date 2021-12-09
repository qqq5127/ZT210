#include "inear_sensor.h"
#include "inear_wq_touch.h"
#include "iot_touch_key.h"
#include "os_timer.h"
#include "touch_key.h"
#include "vendor_msg.h"
#include "lib_dbglog.h"
#include "modules.h"
#include "os_utils.h"
#include "cli.h"

#define APP_CLI_MSG_ID_GET_TOUCH_INEAR_CDC 17

#if INEAR_DRIVER_SELECTION == INEAR_DRIVER_WQ_TOUCH

#ifndef INEAR_TOUCH_LOW_OFFSET
#define INEAR_TOUCH_LOW_OFFSET 7
#endif

#ifndef INEAR_TOUCH_HYSTERESIS_VALUE
#define INEAR_TOUCH_HYSTERESIS_VALUE 6
#endif

#ifndef INEAR_TOUCH_LESS_MAX
#define INEAR_TOUCH_LESS_MAX 2
#endif

#ifndef INEAR_TOUCH_LARGE_MIN
#define INEAR_TOUCH_LARGE_MIN 2
#endif

#ifndef INEAR_CHECK_INTERVAL_FAST
#define INEAR_CHECK_INTERVAL_FAST 2000 /* ms */
#endif

#ifndef INEAR_CHECK_INTERVAL_SLOW
#define INEAR_CHECK_INTERVAL_SLOW 10000 /* ms */
#endif

#ifndef INEAR_PRINT_CDC_INTERVAL
#define INEAR_PRINT_CDC_INTERVAL 20000 /* ms */
#endif

#ifndef INEAR_ENABLE_INTR_INTERVAL
#define INEAR_ENABLE_INTR_INTERVAL 20 /* ms */
#endif

#ifndef INEAR_DEFAULT_OUT_EAR_CDC
#define INEAR_DEFAULT_OUT_EAR_CDC 1100
#endif

#ifndef INEAR_DEFAULT_IN_EAR_CDC
#define INEAR_DEFAULT_IN_EAR_CDC 1200
#endif

#ifndef INEAR_DEFAULT_CHECK_CDC_COUNT
#define INEAR_DEFAULT_CHECK_CDC_COUNT 5
#endif

#define INEAR_WQ_TOUCH_GET_CDC_TIMER 100 /* ms */

typedef struct {
    bool_t dirty;
    uint32_t cdc_max;
    uint32_t cdc_min;
} inear_cfg_t;

typedef struct {
    timer_id_t print_cdc_timer; /** print cdc timer */
    bool_t power_on_cali_done;
    uint32_t cdc_max; /** cdc max value */
    uint32_t cdc_min; /** cdc min value */
} inear_context_t;

typedef enum {
    INEAR_MSG_ID_IN_EAR,
    INEAR_MSG_ID_OUT_OF_EAR,
    INEAR_MSG_ID_ENABLE_INTR,
    INEAR_MSG_ID_TIMER_TIMEOUT,
} inear_msg_id_t;

static bool_t in_ear = false;
static inear_callback_t inear_callback = NULL;
static timer_id_t timer = 0;
static uint8_t tk_id;
static inear_cfg_t inear_cfg;
static inear_context_t _inear_context;
static inear_context_t *inear_context = &_inear_context;

static void inear_wq_touch_timer_dbg_cb(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    uint32_t cdc_value = 0;

    uint8_t ret = iot_touch_key_read_pad_cdc(tk_id, &cdc_value);
    DBGLOG_INEAR_SENSOR_INFO("++++++++++inear timer cdc %d ret = %d.\n", cdc_value, ret);
}

static void touch_key_isr(IOT_TK_PAD_ID pad_id, IOT_TOUCH_KEY_INT int_type)
    IRAM_TEXT(touch_key_isr);
static void touch_key_isr(IOT_TK_PAD_ID pad_id, IOT_TOUCH_KEY_INT int_type)
{
    if (int_type == IOT_TOUCH_KEY_INT_PRESS_MID) {
        vendor_send_msg_from_isr(VENDOR_MSG_TYPE_INEAR, INEAR_MSG_ID_IN_EAR, pad_id);
    } else if (int_type == IOT_TOUCH_KEY_INT_PRESS_RELEASE) {
        vendor_send_msg_from_isr(VENDOR_MSG_TYPE_INEAR, INEAR_MSG_ID_OUT_OF_EAR, pad_id);
    }
}

static void inear_cfg_save(void)
{
    inear_cfg.dirty = true;
    DBGLOG_INEAR_SENSOR_INFO("inear_cfg_save delayed\n");
}

static void timer_process(void)
{
    uint8_t ret;
    static uint8_t cdc_min_count = 0;
    static uint8_t cdc_max_count = 0;
    uint32_t cdc_diff, cdc = 0;
    static uint32_t period = INEAR_CHECK_INTERVAL_FAST;
    static uint32_t cdc_diff_pre = 0;
    static uint32_t loop_count = 0;
    uint32_t cdc1, cdc2;

    ret = iot_touch_key_read_pad_cdc(tk_id, &cdc);
    if (RET_OK != ret) {
        DBGLOG_INEAR_SENSOR_ERROR("get inear touch %d cdc failed, ret = %d.\n", tk_id, ret);
        goto end;
    }

    /* 1. update cdc_max/cdc_min if needed. */
    if ((0 == inear_context->cdc_max) && (0 == inear_context->cdc_min)) {
        inear_context->cdc_max = inear_context->cdc_min = cdc;
        DBGLOG_INEAR_SENSOR_INFO("initial inear cdc:[%d]\n", cdc);
    }

    if (cdc > inear_context->cdc_max) {
        ret = iot_touch_key_read_pad_cdc(tk_id, &cdc1);
        if (RET_OK != ret) {
            DBGLOG_INEAR_SENSOR_ERROR("get inear touch %d cdc1 failed, ret = %d.\n", tk_id, ret);
            goto end;
        }

        ret = iot_touch_key_read_pad_cdc(tk_id, &cdc2);
        if (RET_OK != ret) {
            DBGLOG_INEAR_SENSOR_ERROR("get inear touch %d cdc2 failed, ret = %d.\n", tk_id, ret);
            goto end;
        }

        if ((cdc1 > inear_context->cdc_max) && (cdc2 > inear_context->cdc_max)) {
            inear_context->cdc_max = cdc;
        }
    } else if (cdc < inear_context->cdc_min) {
        ret = iot_touch_key_read_pad_cdc(tk_id, &cdc1);
        if (RET_OK != ret) {
            DBGLOG_INEAR_SENSOR_ERROR("get inear touch %d cdc1 failed, ret = %d.\n", tk_id, ret);
            goto end;
        }

        ret = iot_touch_key_read_pad_cdc(tk_id, &cdc2);
        if (RET_OK != ret) {
            DBGLOG_INEAR_SENSOR_ERROR("get inear touch %d cdc2 failed, ret = %d.\n", tk_id, ret);
            goto end;
        }
        if ((cdc1 < inear_context->cdc_min) && (cdc2 < inear_context->cdc_min)) {
            inear_context->cdc_min = cdc;
        }
    }

    if (!(++loop_count % (INEAR_PRINT_CDC_INTERVAL / period))) {
        /* print cdc every 20s. */
        DBGLOG_INEAR_SENSOR_INFO("in_ear:%d, inear cdc:[%d], cdc_max:[%d], cdc_min:[%d]\n", in_ear,
                                 cdc, inear_context->cdc_max, inear_context->cdc_min);
    }

    /* 2. calibrate thresholds if needed. */
    cdc_diff = inear_context->cdc_max - inear_context->cdc_min;

    if ((cdc_diff != cdc_diff_pre)
        && (cdc_diff > (INEAR_TOUCH_LOW_OFFSET + INEAR_TOUCH_HYSTERESIS_VALUE))) {

        cdc_diff_pre = cdc_diff;
        if (inear_context->power_on_cali_done == false) {
            inear_context->power_on_cali_done = true;
            period = INEAR_CHECK_INTERVAL_SLOW;
        }

        inear_cfg.cdc_min = inear_context->cdc_min;
        inear_cfg.cdc_max = inear_context->cdc_max;

        uint32_t fall_thres = inear_cfg.cdc_min + INEAR_TOUCH_LOW_OFFSET;
        uint32_t climb_thres = fall_thres + INEAR_TOUCH_HYSTERESIS_VALUE;
        DBGLOG_INEAR_SENSOR_INFO("calibrate threshold, climb:[%d], fall:[%d]\n", climb_thres,
                                 fall_thres);
        inear_cfg_save();
        ret = iot_touch_key_change_pad_thrs(tk_id, fall_thres, climb_thres);
        if (RET_OK != ret) {
            DBGLOG_INEAR_SENSOR_ERROR("change inear touch %d thres failed, ret = %d.\n", tk_id,
                                      ret);
        }
    }

#if 1
    /* 3. report in_ear state if needed. */
    if ((cdc <= (inear_cfg.cdc_min + INEAR_TOUCH_LARGE_MIN)) && (in_ear)) {
        /*4.1 continuous read after 100ms if cdc less than min*/
        if (++cdc_min_count <= INEAR_DEFAULT_CHECK_CDC_COUNT) {
            DBGLOG_INEAR_SENSOR_INFO("get cdc value:[%d], inear_cfg.cdc_min:[%d] \n", cdc,
                                     inear_cfg.cdc_min);
            os_stop_timer(timer);
            os_start_timer(timer, 100);
            return;
        }

        cdc_min_count = 0;

        in_ear = false;
        inear_callback(false);
        DBGLOG_INEAR_SENSOR_INFO(
            "inear timer, inear_state: [%d], cdc:[%d], cdc_max: [%d], cdc_min:[%d]\n", in_ear, cdc,
            inear_cfg.cdc_max, inear_cfg.cdc_min);

    } else if ((cdc >= (inear_cfg.cdc_max - INEAR_TOUCH_LESS_MAX)) && (!in_ear)) {
        /*4.2 continuous read after 100ms if cdc large than max*/
        if (++cdc_max_count <= INEAR_DEFAULT_CHECK_CDC_COUNT) {
            DBGLOG_INEAR_SENSOR_INFO("get cdc value:[%d], inear_cfg.cdc_max:[%d] \n", cdc,
                                     inear_cfg.cdc_max);
            os_stop_timer(timer);
            os_start_timer(timer, 100);
            return;
        }

        cdc_max_count = 0;

        in_ear = true;
        inear_callback(true);
        DBGLOG_INEAR_SENSOR_INFO(
            "inear timer, inear_state: [%d], cdc:[%d], cdc_max: [%d], cdc_min:[%d]\n", in_ear, cdc,
            inear_cfg.cdc_max, inear_cfg.cdc_min);
    }
#else
    UNUSED(cdc_min_count);
    UNUSED(cdc_max_count);
#endif

end:
    cdc_min_count = 0;
    cdc_max_count = 0;
    /* 4. restart timer. */
    os_stop_timer(timer);
    os_start_timer(timer, period);
}

static void enable_intr_timer_func(timer_id_t timer_id, void *arg)
{
    UNUSED(arg);
    assert(timer_id == timer);

    vendor_send_msg(VENDOR_MSG_TYPE_INEAR, INEAR_MSG_ID_ENABLE_INTR, 0);
}

static void timer_func(timer_id_t timer_id, void *arg)
{
    UNUSED(arg);
    assert(timer_id == timer);

    vendor_send_msg(VENDOR_MSG_TYPE_INEAR, INEAR_MSG_ID_TIMER_TIMEOUT, 0);
}

static bool_t check_thres_valid(uint32_t climb_thres, uint32_t fall_thres)
{
    return ((climb_thres - fall_thres) >= INEAR_TOUCH_HYSTERESIS_VALUE) ? true : false;
}
static void inear_cfg_load(void)
{
    uint32_t len, ret;

    len = sizeof(inear_cfg);
    ret = storage_read(VENDOR_BASE_ID, VENDOR_CFG_INEAR_TOUCH_THRS, (uint32_t *)&inear_cfg, &len);

    if ((len != sizeof(inear_cfg)) || (ret != RET_OK)) {
        DBGLOG_INEAR_SENSOR_INFO("inear_cfg_load error len:%d ret:%d\n", len, ret);
        inear_cfg.cdc_max = INEAR_DEFAULT_IN_EAR_CDC;
        inear_cfg.cdc_min = INEAR_DEFAULT_OUT_EAR_CDC;
    } else if (!check_thres_valid(inear_cfg.cdc_max, inear_cfg.cdc_min)) {
        DBGLOG_INEAR_SENSOR_INFO("inear_cfg_load, thres(%d:%d) invalid\n", inear_cfg.cdc_max,
                                 inear_cfg.cdc_min);
        inear_cfg.cdc_max = INEAR_DEFAULT_IN_EAR_CDC;
        inear_cfg.cdc_min = INEAR_DEFAULT_OUT_EAR_CDC;
        inear_cfg_save();
    }
}

static void inear_cfg_write_flash(void)
{
    uint32_t ret;

    if (!inear_cfg.dirty) {
        return;
    }

    inear_cfg.dirty = false;
    ret = storage_write(VENDOR_BASE_ID, VENDOR_CFG_INEAR_TOUCH_THRS, (uint32_t *)&inear_cfg,
                        sizeof(inear_cfg));

    DBGLOG_INEAR_SENSOR_INFO("inear_cfg_write_flash ret:%d\n", ret);
}

static void inear_enable_intr(void)
{
    uint8_t ret;

    ret = iot_touch_key_enable_intr(tk_id);
    DBGLOG_INEAR_SENSOR_INFO("inear_touch, enable touch %d intr, ret %d.\n", tk_id, ret);

    if (timer) {
        os_delete_timer(timer);
        timer = 0;

        timer = os_create_timer(IOT_SENSOR_HUB_MANAGER_MID, false, timer_func, NULL);
        assert(timer);
        os_start_timer(timer, 100);

        DBGLOG_INEAR_SENSOR_INFO("re-create inear timer.\n");
    }
}

static void inear_msg_handler(uint8_t msg_id, uint16_t msg_value)
{
    uint32_t cdc = 0;

    UNUSED(msg_value);

    switch (msg_id) {
        case INEAR_MSG_ID_IN_EAR:
            if (!in_ear) {
                in_ear = true;
                inear_callback(true);
            }
            break;
        case INEAR_MSG_ID_OUT_OF_EAR:
            if (in_ear) {
                in_ear = false;
                inear_callback(false);
            }
            break;
        case INEAR_MSG_ID_ENABLE_INTR:
            inear_enable_intr();
            break;
        case INEAR_MSG_ID_TIMER_TIMEOUT:
            timer_process();
            break;
        default:
            DBGLOG_INEAR_SENSOR_INFO("inear_msg_handler err msg_id [%d]\n", msg_id);
            break;
    }

    if ((msg_id == INEAR_MSG_ID_IN_EAR) || (msg_id == INEAR_MSG_ID_OUT_OF_EAR)) {
        iot_touch_key_read_pad_cdc(tk_id, &cdc);
        DBGLOG_INEAR_SENSOR_INFO("inear isr, inear_state: [%d], cdc:[%d]\n",
                                 ((msg_id == INEAR_MSG_ID_IN_EAR) ? 1 : 0), cdc);
    }
}

void inear_wuqi_touch_open(void)
{
    uint8_t ret;
    iot_touch_key_config_t cfg = {0};

    inear_cfg_load();
    cfg.fall_thrs = inear_cfg.cdc_min + INEAR_TOUCH_LOW_OFFSET;
    cfg.climb_thrs = cfg.fall_thrs + INEAR_TOUCH_HYSTERESIS_VALUE;
    cfg.climb_trig_times = IOT_TOUCH_KEY_DEFAULT_CLIMB_TRIG_TIMES;
    cfg.fall_trig_times = IOT_TOUCH_KEY_ANTI_SHAKE_FALL_TRIG_TIMES;

    ret = iot_touch_key_open(tk_id, IOT_TK_ABSOLUTE_MODE, &cfg, touch_key_isr);
    if (RET_OK != ret) {
        DBGLOG_INEAR_SENSOR_ERROR("inear_touch, open tk %d failed, ret = %d.\n", tk_id, ret);
    } else {
        DBGLOG_INEAR_SENSOR_INFO("inear_touch, open tk %d sucess, climb_thres %d, fall_thres %d.\n",
                                 tk_id, cfg.climb_thrs, cfg.fall_thrs);
    }

    timer = os_create_timer(IOT_SENSOR_HUB_MANAGER_MID, false, enable_intr_timer_func, NULL);
    assert(timer);
    os_start_timer(timer, INEAR_ENABLE_INTR_INTERVAL);

    inear_context->print_cdc_timer =
        os_create_timer(IOT_SENSOR_HUB_MANAGER_MID, true, inear_wq_touch_timer_dbg_cb, NULL);
}

void inear_wuqi_touch_cfg_reset(void)
{
    inear_cfg.cdc_max = INEAR_DEFAULT_IN_EAR_CDC;
    inear_cfg.cdc_min = INEAR_DEFAULT_OUT_EAR_CDC;
    inear_cfg_save();
    DBGLOG_INEAR_SENSOR_INFO("inear_cfg_reset done\n");
}

void inear_wuqi_touch_init(inear_callback_t callback)
{
    iot_touch_key_point_num_t point_num = {0};

    inear_callback = callback;

    tk_id = iot_resource_lookup_touch_id(INEAR_TOUCH_PAD_ID);
    if (0xff == tk_id) {
        DBGLOG_INEAR_SENSOR_ERROR("inear_wuqi_touch_init tk_id==0xFF\n");
        return;
    }

    vendor_register_msg_handler(VENDOR_MSG_TYPE_INEAR, inear_msg_handler);

    point_num.monitor_point_num = IOT_TOUCH_KEY_PHASE_POINT_NUM_5;
    point_num.work_point_num = IOT_TOUCH_KEY_PHASE_POINT_NUM_6;
    iot_touch_key_set_pad_info(tk_id, IOT_TK_ABSOLUTE_MODE, &point_num);

    DBGLOG_INEAR_SENSOR_INFO("inear_touch, set touch id %d\n", tk_id);
}

void inear_wuqi_touch_deinit(void)
{
#if 0
    uint8_t ret;

    if (RET_OK != (ret = iot_touch_key_close(tk_id))) {
        DBGLOG_INEAR_SENSOR_ERROR("close inear touch %d failed, ret = %d.\n", tk_id, ret);
    }
#endif

    if (os_is_timer_active(timer)) {
        os_stop_timer(timer);
    }

    inear_cfg_write_flash();
}

static void cli_get_inear_cdc_handler(uint8_t *buffer, uint32_t length)
{
    uint32_t cdc;

    UNUSED(buffer);

    if (length != 0) {
        DBGLOG_INEAR_SENSOR_ERROR("cli_get_inear_cdc_handler invalid length:%d\n", length);
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSG_ID_GET_TOUCH_INEAR_CDC,
                                   NULL, 0, 0, RET_FAIL);
        return;
    }

    if (0xff == tk_id) {
        DBGLOG_INEAR_SENSOR_ERROR("cli_get_inear_cdc_handler tk_id==0xFF\n");
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSG_ID_GET_TOUCH_INEAR_CDC,
                                   NULL, 0, 0, RET_FAIL);
        return;
    }

    if (RET_OK != iot_touch_key_read_pad_cdc(tk_id, &cdc)) {
        DBGLOG_INEAR_SENSOR_ERROR("cli_get_inear_cdc_handler get %d cdc failed\n", tk_id);
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSG_ID_GET_TOUCH_INEAR_CDC,
                                   NULL, 0, 0, RET_FAIL);
        return;
    }

    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSG_ID_GET_TOUCH_INEAR_CDC,
                               (uint8_t *)&cdc, sizeof(cdc), 0, RET_OK);

    if (os_is_timer_active(inear_context->print_cdc_timer)) {
        os_stop_timer(inear_context->print_cdc_timer);
    } else {
        os_start_timer(inear_context->print_cdc_timer, INEAR_WQ_TOUCH_GET_CDC_TIMER);
    }
}

CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSG_ID_GET_TOUCH_INEAR_CDC,
                cli_get_inear_cdc_handler);
#endif   //INEAR_DRIVER_SELECTION == INEAR_DRIVER_WQ_TOUCH
