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

/* os shim includes */
#include "types.h"
#include "string.h"

/* common includes */
#include "iot_share_task.h"
#include "os_task.h"
#include "os_event.h"
#include "os_queue.h"
#include "riscv_cpu.h"

#ifdef BUILD_CORE_CORE0
#define SHARE_TASK_FAST_QUEUE_MSG_ID_TYPE   7
#define SHARE_TASK_SLOW_QUEUE_MSG_ID_TYPE   3
#else
#define SHARE_TASK_FAST_QUEUE_MSG_ID_TYPE   6
#define SHARE_TASK_SLOW_QUEUE_MSG_ID_TYPE   2
#endif

#define SHARE_TASK_MSG_ID_TYPE (SHARE_TASK_FAST_QUEUE_MSG_ID_TYPE + SHARE_TASK_SLOW_QUEUE_MSG_ID_TYPE)
#define SHARE_TASK_FAST_QUEUE_LENGTH (SHARE_TASK_FAST_QUEUE_MSG_ID_TYPE * 8)
#define SHARE_TASK_SLOW_QUEUE_LENGTH (SHARE_TASK_SLOW_QUEUE_MSG_ID_TYPE * 8)

#define EVENT_TYPE_VALID(tp) \
        ((tp) >= IOT_SHARE_EVENT_START && (tp) < IOT_SHARE_EVENT_END)

struct func_args {
    iot_share_event_func func;
    void *arg;
};

struct share_task {
    const char *name;
    uint8_t prio;
    os_task_h task;                 /* The higher priority task */
    os_event_h event;
    struct func_args handlers[IOT_SHARE_EVENT_END];
};

typedef struct _iot_share_twins_task {
    struct share_task faster;       /* The higher priority task */
    struct share_task slower;       /* The lower priority task */
} iot_share_twins_task;

static iot_share_twins_task twins_task = {
    // faster
    {
        .name = "share_task_fast",
        .prio = 9,
        .task = NULL,
    },
    // slower
    {
        .name = "share_task_slow",
        .prio = 6,
        .task = NULL,
    },
};

static os_queue_h msg_faster;
static os_queue_h msg_slower;

struct msg_id_info {
    iot_share_event_func func;
};
static struct msg_id_info msg_id_queue[SHARE_TASK_MSG_ID_TYPE] = {0};

struct queue_item {
    int32_t msg_id;
    void *data;
};

static inline struct share_task *iot_share_task_route_task(uint32_t prio)
{
    return (IOT_SHARE_TASK_QUEUE_HP == prio) ?
            &twins_task.faster : &twins_task.slower;
}

//lint -sem(iot_share_task_msg_handler, thread_protected)
static void iot_share_task_msg_handler(void *arg)
{
    os_queue_h queue = arg;
    struct queue_item item;
    iot_share_event_func exec_func;

    while (os_queue_peek(queue, &item)) {
        assert(item.msg_id < SHARE_TASK_MSG_ID_TYPE);

        exec_func = msg_id_queue[item.msg_id].func;
        assert(exec_func);

        exec_func(item.data);
    }
}

bool_t iot_share_task_post_msg(uint32_t prio, int32_t msg_id, void *data)
{
    bool_t ret;
    struct queue_item item = { msg_id, data };
    static os_queue_h msg;
    uint32_t dump_param[5];

    if (prio == IOT_SHARE_TASK_QUEUE_HP) {
        msg = msg_faster;
    } else {
        msg = msg_slower;
    }

    ret = os_queue_send(msg, &item);
    /* The queue might be full, please check whether the queue length is not
     * enough or iot_share_task_msg_handler has no oppotunity to execute. */
    if (!ret) {
        dump_param[0] = cpu_get_mhartid();                      /* core */
        dump_param[1] = prio;                                   /* msg_faster or msg_slower */
        dump_param[2] = (uint32_t)msg_id_queue[msg_id].func;    /* func address */
        dump_param[3] = (uint32_t)data;                         /* data */
        dump_param[4] = os_queue_items_get(msg);                /* current queue size */
        ASSERT_FAILED_DUMP(dump_param, 5);
    }
    iot_share_task_post_event(prio, IOT_SHARE_EVENT_MSG_EVENT);

    return true;
}

bool_t iot_share_task_post_msg_from_isr(uint32_t prio, int32_t msg_id, void *data) IRAM_TEXT(iot_share_task_post_msg_from_isr);
bool_t iot_share_task_post_msg_from_isr(uint32_t prio, int32_t msg_id, void *data)
{
    bool_t ret;
    struct queue_item item = { msg_id, data };
    static os_queue_h msg;
    uint32_t dump_param[5];

    if (prio == IOT_SHARE_TASK_QUEUE_HP) {
        msg = msg_faster;
    } else {
        msg = msg_slower;
    }

    ret = os_queue_send_from_isr(msg, &item);
    /* The queue might be full, please check whether the queue length is not
     * enough or iot_share_task_msg_handler has no oppotunity to execute. */
    if (!ret) {
        dump_param[0] = cpu_get_mhartid();                      /* core */
        dump_param[1] = prio;                                   /* msg_faster or msg_slower */
        dump_param[2] = (uint32_t)msg_id_queue[msg_id].func;    /* func address */
        dump_param[3] = (uint32_t)data;                         /* data */
        dump_param[4] = os_queue_items_get(msg);                /* current queue size */
        ASSERT_FAILED_DUMP(dump_param, 5);
    }
    iot_share_task_post_event_from_isr(prio, IOT_SHARE_EVENT_MSG_EVENT);

    return true;
}

static int32_t msg_id_bind(iot_share_event_func exec_func)
{
    int i;

    // validable num begin from 1
    for (i = 1; i < SHARE_TASK_MSG_ID_TYPE; i++) {
        if (!msg_id_queue[i].func) {
            msg_id_queue[i].func = exec_func;
            return i;
        }
    }

    /* SHARE_TASK_MSG_ID_TYPE is too small-scale.
     * Please check the sum of the two values:
     *   1. SHARE_TASK_FAST_QUEUE_MSG_ID_TYPE
     *   2. SHARE_TASK_SLOW_QUEUE_MSG_ID_TYPE
     */
    assert(0);
    return 0;   //lint !e527 reachable if disable assert
}

static void msg_id_unbind(int32_t msg_id)
{
    msg_id_queue[msg_id].func = NULL;
}

int32_t iot_share_task_msg_register(iot_share_event_func exec_func)
{
    return msg_id_bind(exec_func);
}

void iot_share_task_msg_unregister(int32_t msg_id)
{
    msg_id_unbind(msg_id);
}

uint32_t iot_share_task_post_event(uint32_t prio,
                                    iot_share_event_type type)
{
    bool_t ret;
    struct share_task *task = iot_share_task_route_task(prio);
    if (!task) {
        return RET_FAIL;
    }

    ret = os_set_event(task->event, BIT(type));
    if (!ret) {
        return RET_FAIL;
    }

    return RET_OK;
}

uint32_t iot_share_task_post_event_from_isr(uint32_t prio,
                                            iot_share_event_type type) IRAM_TEXT(iot_share_task_post_event_from_isr);
uint32_t iot_share_task_post_event_from_isr(uint32_t prio,
                                            iot_share_event_type type)
{
    bool_t ret;
    struct share_task *task = iot_share_task_route_task(prio);
    if (!task) {
        return RET_FAIL;
    }

    ret = os_set_event_isr(task->event, BIT(type));
    if (!ret) {
        return RET_FAIL;
    }

    return RET_OK;
}

uint32_t iot_share_task_event_register(uint32_t prio,
                                        iot_share_event_type type,
                                        iot_share_event_func func,
                                        void *arg)
{
    struct share_task *task = iot_share_task_route_task(prio);
    if (!task) {
        return RET_FAIL;
    }

    if (!EVENT_TYPE_VALID(type)) {
        return RET_FAIL;
    }

    struct func_args *handler = &task->handlers[type];

    if (handler->func && handler->func != func) {
        return RET_FAIL;
    }

    handler->func = func;
    handler->arg = arg;

    return RET_OK;
}

uint32_t iot_share_task_event_unregister(uint32_t prio,
                                        iot_share_event_type type)
{
    struct share_task *task = iot_share_task_route_task(prio);
    if (!task)
        return RET_FAIL;

    if (!EVENT_TYPE_VALID(type)) {
        return RET_FAIL;
    }

    struct func_args *handler = &task->handlers[type];

    handler->func = NULL;
    handler->arg = NULL;

    return RET_OK;
}

static void iot_share_task_handle_event(struct share_task *task, uint32_t event)
{
    struct func_args *entry;
    uint32_t type, mask;

    for (type = IOT_SHARE_EVENT_START; type < IOT_SHARE_EVENT_END; type++) {
        mask = BIT(type);
        if (mask & event) {
            entry = &task->handlers[type];

            if (entry->func)
                entry->func(entry->arg);
        }
    }

    return;
}

static void iot_share_task_func(void *arg)
{
    bool_t ret;
    uint32_t events;
    struct share_task *task = arg;

    while (1) {     //lint !e716 task main loop
        /* waiting for messages and task events.
         * always handle takes event firstly.
         */
        ret = os_wait_event(task->event, MAX_TIME, &events);
        if (ret) {
            iot_share_task_handle_event(task, events);
        }
    }
}

static uint32_t create_share_task(struct share_task *task)
{
    os_task_h p_task;
    os_event_h p_event;

    p_event = os_create_event(IOT_SHARETASK_MID);
    if (!p_event) {
        return RET_FAIL;
    }

    task->event = p_event;

    p_task = os_create_task_ext(iot_share_task_func,
                                task, task->prio, 0, task->name);
    if (!p_task)
        return RET_FAIL;

    task->task = p_task;

    memset(task->handlers, 0, sizeof(struct func_args) * IOT_SHARE_EVENT_END);
    return RET_OK;
}

static void delete_share_task(const struct share_task *task)
{
    os_delete_event(task->event);
    os_delete_task(task->task);
}

uint32_t iot_share_task_init(void)
{
    uint32_t ret;

    if (twins_task.faster.task && twins_task.slower.task) {
        return RET_FAIL;
    }

    ret = create_share_task(&twins_task.faster);
    if (ret != RET_OK) {
        return RET_FAIL;
    }

    ret = create_share_task(&twins_task.slower);
    if (ret != RET_OK) {
        return RET_FAIL;
    }

#if (SHARE_TASK_FAST_QUEUE_LENGTH > 0)
    msg_faster = os_queue_create(IOT_SHARETASK_MID,
                                    SHARE_TASK_FAST_QUEUE_LENGTH,
                                    sizeof(struct queue_item));
    if (!msg_faster) {
        return RET_FAIL;
    }

    ret = iot_share_task_event_register(IOT_SHARE_TASK_QUEUE_HP,
                                        IOT_SHARE_EVENT_MSG_EVENT,
                                        iot_share_task_msg_handler,
                                        msg_faster);
    if (ret != RET_OK) {
        return RET_FAIL;
    }
#endif

#if (SHARE_TASK_SLOW_QUEUE_LENGTH > 0)
    msg_slower = os_queue_create(IOT_SHARETASK_MID,
                                    SHARE_TASK_SLOW_QUEUE_LENGTH,
                                    sizeof(struct queue_item));
    if (!msg_slower) {
        return RET_FAIL;
    }

    ret = iot_share_task_event_register(IOT_SHARE_TASK_QUEUE_LP,
                                        IOT_SHARE_EVENT_MSG_EVENT,
                                        iot_share_task_msg_handler,
                                        msg_slower);
    if (ret != RET_OK) {
        return RET_FAIL;
    }
#endif

    return RET_OK;
}

void iot_share_task_deinit(void)
{
    delete_share_task(&twins_task.slower);
    delete_share_task(&twins_task.faster);
}
