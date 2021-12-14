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

/*
 * INCLUDE FILES
 ****************************************************************************
 */
#include "types.h"
#include "stdio.h"
#include "os_task.h"
#include "os_queue.h"
#include "os_mem.h"
#include "os_timer.h"
#include "os_lock.h"
#include "string.h"
#include "app_main.h"
#include "app_pm.h"
#include "app_charger.h"
#include "app_btn.h"
#include "app_led.h"
#include "app_bt.h"
#include "app_evt.h"
#include "app_tone.h"
#include "app_inear.h"
#include "usr_cfg.h"
#include "ro_cfg.h"
#include "app_conn.h"
#include "app_bat.h"
#include "app_wws.h"
#include "app_audio.h"
#include "app_gatts.h"
#include "app_econn.h"
#include "app_cli.h"
#include "app_wqota.h"
#include "app_ota_sync.h"

/*
 * MACROS
 ****************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************
 */
#ifndef MAX_DELAYED_MSG_COUNT
#define MAX_DELAYED_MSG_COUNT 16
#endif

#ifndef APP_MAIN_TASK_PRIO
#define APP_MAIN_TASK_PRIO 6
#endif

#ifndef APP_MAIN_STACK_SIZE
#define APP_MAIN_STACK_SIZE 2048
#endif

#ifndef APP_MSG_QUEUE_SIZE
#define APP_MSG_QUEUE_SIZE 64
#endif

#define APP_MAIN_MSG_ID_TIMER_TRIGGER 1

/*
 * ENUMERATIONS
 ****************************************************************************
 */

/*
 * TYPE DEFINITIONS
 ****************************************************************************
 */
typedef struct {
    uint16_t type;
    uint16_t id;
    uint8_t param[];
} app_msg_t;

typedef struct {
    timer_id_t tid;
    bool_t used;
    app_msg_t *msg;
} timer_context_t;
/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************
 */
static os_task_h main_task_handle = NULL;
static os_queue_h main_queue_handle = NULL;
static app_msg_handler_t msg_handlers[MSG_TYPE_MAX] = {0};
static timer_context_t timers[MAX_DELAYED_MSG_COUNT] = {0};
static volatile bool_t timer_handling = false;

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************
 */
static void timer_handler(timer_id_t timer_id, void *arg)
{
    timer_context_t *timer = arg;
    UNUSED(timer_id);

    timer_handling = true;
    if (app_send_msg(MSG_TYPE_MAIN, APP_MAIN_MSG_ID_TIMER_TRIGGER, &timer,
                     sizeof(timer_context_t *))) {
        DBGLOG_MAIN_ERR("send APP_MAIN_MSG_ID_TIMER_TRIGGER failed\n");
        os_delete_timer(timer->tid);
        if (timer->msg) {
            os_mem_free(timer->msg);
        }
        timer->msg = 0;
        timer->tid = 0;
        timer->used = 0;
    }
    timer_handling = false;
}

static void handle_msg(app_msg_t *msg)
{
    assert(msg);
    if (msg->type != MSG_TYPE_MAIN) {
        DBGLOG_MAIN_DBG("handle_msg type:%d id:%d\n", msg->type, msg->id);
    }

    if (app_bt_is_in_ft_mode()) {
        DBGLOG_MAIN_DBG("factory test mode, message ignored\n");
        return;
    }

    if (msg->type >= MSG_TYPE_MAX) {
        DBGLOG_MAIN_ERR("handle_msg type:%d error!\n", msg->type);
    } else {
        if (msg_handlers[msg->type]) {
            msg_handlers[msg->type](msg->id, msg->param);
        } else {
            DBGLOG_MAIN_ERR("handle_msg type:%d handler not found!\n", msg->type);
        }
    }
}

static void app_main_handle_msg(uint16_t msg_id, void *param)
{
    switch (msg_id) {
        case APP_MAIN_MSG_ID_TIMER_TRIGGER: {
            app_msg_t *msg = NULL;
            timer_context_t *timer = *((timer_context_t **)param);
            if (timer->tid) {
                os_delete_timer(timer->tid);
                timer->tid = 0;
            }
            if (timer->msg) {
                msg = timer->msg;
                timer->msg = NULL;
            }
            timer->used = 0;
            if (msg) {
                handle_msg(msg);
                os_mem_free(msg);
            }
            break;
        }
        default:
            break;
    }
}

static void app_main_task_func(void *arg)
{
    app_msg_t *msg;

    UNUSED(arg);

    app_register_msg_handler(MSG_TYPE_MAIN, app_main_handle_msg);

    app_bt_init();
    ro_cfg_init();
    usr_cfg_init();
    app_wws_init();
    app_evt_init();
    app_btn_init();
    app_pm_init();
    app_conn_init();
    app_led_init();
    app_bat_init();
    app_charger_init();
    app_tone_init();
    app_audio_init();
    app_inear_init();
    app_gatts_init();
    app_econn_init();
    app_cli_init();
    app_wqota_init();
    app_ota_sync_init();

    while (1) {
        if (os_queue_receive(main_queue_handle, &msg)) {
            handle_msg(msg);
            os_mem_free(msg);
        }
    }
}

uint32_t app_main_entry(void)
{
    DBGLOG_MAIN_DBG("app_main_entry\n");

    main_queue_handle = os_queue_create(IOT_APP_MID, APP_MSG_QUEUE_SIZE, sizeof(app_msg_t *));
    assert(main_queue_handle);

    main_task_handle = os_create_task_ext(app_main_task_func, NULL, APP_MAIN_TASK_PRIO,
                                          APP_MAIN_STACK_SIZE / 4, "app_main");
    assert(main_task_handle);

    // get dbglog config from flash. should be called after storage init done
    // and dsp and core1 is up.
    dbglog_config_load();
    return 0;
}

void app_deinit(void)
{
    app_wqota_deinit();
    app_cli_deinit();
    app_econn_deinit();
    app_gatts_deinit();
    app_inear_deinit();
    app_btn_deinit();
    app_tone_deinit();
    app_charger_deinit();
    app_bat_deinit();
    app_led_deinit();
    app_conn_deinit();
    app_pm_deinit();
    app_evt_deinit();
    app_wws_deinit();
    app_bt_deinit();
    app_audio_deinit();
    usr_cfg_deinit();
    ro_cfg_deinit();
}

static app_msg_t *gen_msg(app_msg_type_t type, uint16_t id, const void *param, uint16_t param_len)
{
    app_msg_t *msg;

    msg = os_mem_malloc(IOT_APP_MID, sizeof(app_msg_t) + param_len);

    if (!msg) {
        return NULL;
    }

    msg->type = type;
    msg->id = id;
    if (param_len && param) {
        memcpy(msg->param, param, param_len);
    }

    return msg;
}

int app_send_msg(app_msg_type_t type, uint16_t id, const void *param, uint16_t param_len)
{
    app_msg_t *msg;

    msg = gen_msg(type, id, param, param_len);
    if (!msg) {
        DBGLOG_MAIN_ERR("app_send_msg type:%d id:%d gen_msg error\n", type, id);
        return RET_FAIL;
    }

    if (!os_queue_send(main_queue_handle, &msg)) {
        DBGLOG_MAIN_ERR("app_send_msg type:%d id:%d os_queue_send error\n", type, id);
        os_mem_free(msg);
        return RET_FAIL;
    }

    return 0;
}

int app_send_msg_delay(app_msg_type_t type, uint16_t id, const void *param, uint16_t param_len,
                       uint32_t delay_ms)
{
    timer_context_t *timer = NULL;

    if ((type < 0) || (type >= MSG_TYPE_MAX)) {
        DBGLOG_MAIN_ERR("app_send_msg_delay type:%d error!\n", type);
        return RET_FAIL;
    }

    if (delay_ms == 0) {
        return app_send_msg(type, id, param, param_len);
    }

    assert(os_get_current_task_handle() == main_task_handle);

    for (int i = 0; i < MAX_DELAYED_MSG_COUNT; i++) {
        if (!timers[i].used) {
            continue;
        }
        if (!timers[i].msg) {
            continue;
        }
        if ((timers[i].msg->type == type) && (timers[i].msg->id == id)) {
            timer = &timers[i];
            break;
        }
    }

    if (timer) {
        DBGLOG_MAIN_ERR("app_send_msg_delay %d:%d failed, already exists\n", type, id);
        return RET_FAIL;
    }

    for (int i = 0; i < MAX_DELAYED_MSG_COUNT; i++) {
        if (!timers[i].used) {
            timer = &timers[i];
            timer->used = true;
            break;
        }
    }

    if (!timer) {
        DBGLOG_MAIN_ERR("app_send_msg_delay alloc timer for %d:%d failed\n", type, id);
        return RET_FAIL;
    }

    app_msg_t *msg = gen_msg(type, id, param, param_len);
    if (!msg) {
        DBGLOG_MAIN_ERR("app_send_msg_delay %d:%d gen_msg failed\n", type, id);
        return RET_FAIL;
    }

    timer->tid = os_create_timer(IOT_APP_MID, false, timer_handler, timer);
    if (timer->tid) {
        timer->msg = msg;
        os_start_timer(timer->tid, delay_ms);
    } else {
        DBGLOG_MAIN_ERR("os create timer for %d:%d failed\n", type, id);
        timer->used = false;
        os_mem_free(msg);
    }

    DBGLOG_MAIN_DBG("app_send_msg_delay type:%d id:%d delay:%d\n", type, id, delay_ms);
    return 0;
}

int app_cancel_msg(app_msg_type_t type, uint16_t id)
{
    timer_context_t *timer = NULL;
    app_msg_t *msg = NULL;

    assert(os_get_current_task_handle() == main_task_handle);

    for (int i = 0; i < MAX_DELAYED_MSG_COUNT; i++) {
        if (!timers[i].used) {
            continue;
        }
        if (!timers[i].msg) {
            continue;
        }
        if ((timers[i].msg->type == type) && (timers[i].msg->id == id)) {
            timer = &timers[i];
            break;
        }
    }

    if (timer) {
        assert(!timer_handling);
        if (timer->tid) {
            os_stop_timer(timer->tid);
            os_delete_timer(timer->tid);
            timer->tid = 0;
        }
        if (timer->msg) {
            msg = timer->msg;
            timer->msg = NULL;
        }
        timer->used = 0;
        if (msg) {
            os_mem_free(msg);
        }
        DBGLOG_MAIN_DBG("app_cancel_msg type:%d id:%d\n", type, id);
    }

    app_msg_t *queue_head = os_mem_malloc(IOT_APP_MID, sizeof(app_msg_t));
    queue_head->type = MSG_TYPE_MAX;
    queue_head->id = 0;

    if (!os_queue_send(main_queue_handle, &queue_head)) {
        DBGLOG_MAIN_ERR("app_cancel_msg queue_head os_queue_send error\n");
        os_mem_free(queue_head);
        queue_head = NULL;
    }

    for (int i = 0; i < APP_MSG_QUEUE_SIZE; i++) {
        if (!os_queue_peek(main_queue_handle, &msg)) {
            break;
        }
        if (msg == queue_head) {
            os_mem_free(queue_head);
            break;
        }
        if ((msg->type == type) && (msg->id == id)) {
            os_mem_free(msg);
            continue;
        }
        if ((msg->type == MSG_TYPE_MAIN) && (msg->id == APP_MAIN_MSG_ID_TIMER_TRIGGER)) {
            timer_context_t *msg_timer = *((timer_context_t **)msg->param);
            if (msg_timer == timer) {
                os_mem_free(msg);
                continue;
            }
        }
        if (!os_queue_send(main_queue_handle, &msg)) {
            DBGLOG_MAIN_ERR("app_cancel_msg os_queue_send %d:%d error\n", msg->type, msg->id);
            os_mem_free(msg);
        }
    }

    return 0;
}

int app_register_msg_handler(app_msg_type_t type, app_msg_handler_t handler)
{
    if ((type < 0) || (type >= MSG_TYPE_MAX)) {
        DBGLOG_MAIN_ERR("app_register_msg_handler type:%d error!\n", type);
        return RET_FAIL;
    }

    msg_handlers[type] = handler;
    return RET_OK;
}

void app_handle_pending_message(void)
{
    app_msg_t *msg;
    while (os_queue_peek(main_queue_handle, &msg)) {
        handle_msg(msg);
        os_mem_free(msg);
    }
}
