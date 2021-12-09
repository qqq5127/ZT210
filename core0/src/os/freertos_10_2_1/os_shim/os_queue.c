/****************************************************************************

Copyright(c) 2016 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/

/* Free RTOS includes */
#include "FreeRTOS.h"
#include "queue.h"

/* os shim includes */
#include "os_mem.h"
#include "os_queue.h"

#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
#include "riscv_cpu.h"
#endif

typedef struct _os_queue {
    QueueHandle_t queue;
} os_queue_t;

os_queue_h os_queue_create(module_id_t module_id, uint32_t queue_lenth, uint32_t item_size)
{
    os_queue_t *tmp;
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
    uint32_t ret_addr;

    ret_addr = CPU_GET_RET_ADDRESS();

    tmp = os_mem_malloc_dbg(module_id, sizeof(*tmp), ret_addr);
#else
    tmp = os_mem_malloc(module_id, sizeof(*tmp));
#endif
    if (tmp) {
        tmp->queue = xQueueCreate(queue_lenth, item_size);
        if (tmp->queue == NULL) {
            os_mem_free(tmp);
            tmp = NULL;
        }
    }
    return tmp;
}

bool_t os_queue_send(os_queue_h queue, void *queue_tx_msg)
{
    BaseType_t result;

    if (queue_tx_msg == NULL) {
        assert(0);
    }

    result = xQueueSend(((os_queue_t *)queue)->queue, queue_tx_msg, 0);
    return result == pdPASS ? true : false;
} /*lint !e818 Can't be pointer to const due to the typedef. */

bool_t os_queue_send_from_isr(os_queue_h queue, void *queue_tx_msg)
    IRAM_TEXT(os_queue_send_from_isr);
bool_t os_queue_send_from_isr(os_queue_h queue, void *queue_tx_msg)
{
    BaseType_t result;
    BaseType_t xHigherPriorityTaskWoken;

    if (queue_tx_msg == NULL) {
        assert(0);
    }

    // We have not woken a task at the start of the ISR.
    xHigherPriorityTaskWoken = pdFALSE;

    result =
        xQueueSendFromISR(((os_queue_t *)queue)->queue, queue_tx_msg, &xHigherPriorityTaskWoken);
    return result == pdPASS ? true : false;
} /*lint !e818 Can't be pointer to const due to the typedef. */

bool_t os_queue_receive(os_queue_h queue, void *queue_rx_msg)
{
    BaseType_t result;
    if (queue_rx_msg == NULL) {
        assert(0);
    }

    result = xQueueReceive(((os_queue_t *)queue)->queue, queue_rx_msg, portMAX_DELAY);
    return result == pdPASS ? true : false;
} /*lint !e818 Can't be pointer to const due to the typedef. */

bool_t os_queue_receive_timeout(os_queue_h queue, void *queue_rx_msg, uint32_t timeout)
{
    BaseType_t result;

    if (queue_rx_msg == NULL) {
        assert(0);
    }

    if (timeout != portMAX_DELAY) {
        timeout = pdMS_TO_TICKS(timeout);
        if (timeout == portMAX_DELAY) {
            timeout--;
        }
    }

    result = xQueueReceive(((os_queue_t *)queue)->queue, queue_rx_msg, timeout);
    return result == pdPASS ? true : false;
} /*lint !e818 Can't be pointer to const due to the typedef. */

bool_t os_queue_peek(os_queue_h queue, void *queue_rx_msg)
{
    BaseType_t result;

    if (queue_rx_msg == NULL) {
        assert(0);
    }

    result = xQueueReceive(((os_queue_t *)queue)->queue, queue_rx_msg, 0);
    return result == pdPASS ? true : false;
} /*lint !e818 Can't be pointer to const due to the typedef. */

bool_t os_queue_receive_from_isr(os_queue_h queue, void *queue_rx_msg)
{
    BaseType_t result;
    BaseType_t xTaskWokenByReceive = pdFALSE;

    if (queue_rx_msg == NULL) {
        assert(0);
    }

    result = xQueueReceiveFromISR(((os_queue_t *)queue)->queue, queue_rx_msg, &xTaskWokenByReceive);
    return result == pdPASS ? true : false;
} /*lint !e818 Can't be pointer to const due to the typedef. */

uint32_t os_queue_items_get(os_queue_h queue)
{
    UBaseType_t result;
    result = uxQueueMessagesWaitingFromISR(((os_queue_t *)queue)->queue);

    return (uint32_t)result;
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_queue_delete(os_queue_h queue)
{
    vQueueDelete(((os_queue_t *)queue)->queue);
    os_mem_free(queue);
} /*lint !e818 Can't be pointer to const due to the typedef. */
