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
#include "vendor_msg.h"
#include "iot_share_task.h"
#include "lib_dbglog.h"

static bool_t inited = false;
static uint32_t share_msg_id = 0;
static vendor_msg_handler_t handlers[VENDOR_MSG_TYPE_MAX] = {0};

typedef struct {
    uint8_t type;
    uint8_t id;
    uint16_t value;
} share_msg_t;

static void share_msg_func(void *param)
{
    uint32_t the_param = (uint32_t)param;
    share_msg_t *msg = (share_msg_t *)&the_param;

    if (msg->type >= VENDOR_MSG_TYPE_MAX) {
        DBGLOG_VENDOR_MESSAGE_ERROR("vendor share_msg_func type:%d error\n", msg->type);
        return;
    }

    if (handlers[msg->type]) {
        handlers[msg->type](msg->id, msg->value);
    }
}

static void vendor_msg_init(void)
{
    if (inited) {
        return;
    }
    share_msg_id = iot_share_task_msg_register(share_msg_func);

    if (share_msg_id == 0) {
        DBGLOG_VENDOR_MESSAGE_ERROR("vendor_msg_init register failed");
        return;
    }

    inited = true;
}

void vendor_register_msg_handler(vendor_msg_type_t type, vendor_msg_handler_t handler)
{
    if (!inited) {
        vendor_msg_init();
    }

    if (type >= VENDOR_MSG_TYPE_MAX) {
        DBGLOG_VENDOR_MESSAGE_ERROR("vendor_register_msg_handler type:%d error\n", type);
        return;
    }

    handlers[type] = handler;
}

static bool_t send_msg_common(uint32_t prio, vendor_msg_type_t type, uint8_t msg_id, uint16_t msg_value)
{
    share_msg_t msg;
    uint32_t the_param;

    msg.type = type;
    msg.id = msg_id;
    msg.value = msg_value;

    the_param = *((uint32_t *)&msg);

    return iot_share_task_post_msg(prio, share_msg_id, (void *)the_param);
}

bool_t vendor_send_msg(vendor_msg_type_t type, uint8_t msg_id, uint16_t msg_value)
{
    return send_msg_common(IOT_SHARE_TASK_QUEUE_HP, type, msg_id, msg_value);
}

bool_t vendor_send_msg_from_isr(vendor_msg_type_t type, uint8_t msg_id, uint16_t msg_value)
                                                        IRAM_TEXT(vendor_send_msg_from_isr);
bool_t vendor_send_msg_from_isr(vendor_msg_type_t type, uint8_t msg_id, uint16_t msg_value)
{
    share_msg_t msg;
    uint32_t the_param;

    msg.type = type;
    msg.id = msg_id;
    msg.value = msg_value;

    the_param = *((uint32_t *)&msg);

    return iot_share_task_post_msg_from_isr(IOT_SHARE_TASK_QUEUE_HP, share_msg_id,
                                            (void *)the_param);
}
