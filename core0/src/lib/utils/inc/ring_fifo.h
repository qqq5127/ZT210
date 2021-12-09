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

#ifndef GENERIC_QUEUE_H
#define GENERIC_QUEUE_H

#include "atomic.h"

struct queue {
    uint32_t unit_size;
    uint32_t capacity;
    uint8_t *data;
    uint32_t reader;
    uint32_t writer;
};

/**
 * @brief This function is to init queue.
 *
 * @param q is queue
 * @param len is queue length, and the max capacity of members equals (length/unit_size - 1)
 * @param unit_size is queue unit size
 * @return int32_t RET_OK for success else for error
 */
int32_t init_queue(struct queue *q, uint32_t len, uint32_t unit_size);

/**
 * @brief This function is to enqueue.
 *
 * @param q is queue pointer
 * @param data is enqueue data
 * @return int32_t RET_OK for success else for error
 */
int32_t enqueue(struct queue *q, const void *data);

/**
 * @brief This function is dequeue.
 *
 * @param q is queue
 * @param data is dequeue data
 * @return int32_t RET_OK for success else for error
 */
int32_t dequeue(struct queue *q, void *data);

/**
 * @brief This function is to check if the queue is full
 *
 * @param q is queue
 * @return true queue is full
 * @return false queue is not full
 */
bool queue_is_full(const struct queue *q);

/**
 * @brief This function is to check if the queue is empty
 *
 * @param q is queue
 * @return true queue is empty
 * @return false queue is not empty
 */
bool queue_is_empty(const struct queue *q);

#endif
