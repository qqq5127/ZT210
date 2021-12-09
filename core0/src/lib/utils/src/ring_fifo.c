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

#include "errno.h"
#include "types.h"
#include "ring_fifo.h"
#include "string.h"
#include "critical_sec.h"

int32_t init_queue(struct queue *q, uint32_t len, uint32_t unit_size)
{
    if (len % unit_size) {
        return RET_INVAL;
    }

    q->capacity = len;
    q->unit_size = unit_size;
    q->reader = 0;
    q->writer = 0;

    return RET_OK;
}

static uint32_t queue_next(const struct queue *q, uint32_t pos)
{
    uint32_t next = pos + q->unit_size;
    next = (next >= q->capacity) ? (next - q->capacity) : next;
    return next;
}

bool queue_is_full(const struct queue *q) IRAM_TEXT(queue_is_full);
bool queue_is_full(const struct queue *q)
{
    return queue_next(q, q->writer) == q->reader;
}

bool queue_is_empty(const struct queue *q)
{
    return q->writer == q->reader;
}

int32_t enqueue(struct queue *q, const void *data) IRAM_TEXT(enqueue);
int32_t enqueue(struct queue *q, const void *data)
{
    if (!q->data) {
        return RET_NOMEM;
    }

    cpu_critical_enter();

    if (queue_is_full(q)) {
        cpu_critical_exit();
        return RET_BUSY;
    }

    memcpy(q->data + q->writer, data, q->unit_size);
    q->writer = queue_next(q, q->writer);

    cpu_critical_exit();
    return RET_OK;
}

int32_t dequeue(struct queue *q, void *data)
{
    if (!q->data) {
        return RET_NOMEM;
    }

    cpu_critical_enter();

    if (queue_is_empty(q)) {
        cpu_critical_exit();
        return RET_FAIL;
    }

    memcpy(data, q->data + q->reader, q->unit_size);
    q->reader = queue_next(q, q->reader);

    cpu_critical_exit();
    return RET_OK;
}
