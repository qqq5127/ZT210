
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
#ifndef LIB_MEM_MONITOR_H
#define LIB_MEM_MONITOR_H

#include "types.h"
#include "string.h"

#define MAX_MONITOR_NODE  16

/**
 * @brief This is memory detect node callback function defination.
 * @note OS_relate API can not be used in callback funtion, should not disable/enable IRQ in callback
 * @param arg node check callback function argument
 *
 * @return true means register callback function ok,otherwise is false
 */
typedef bool_t (*node_check_callback)(void* arg);

/**
 * @brief This function is to register a memory detect node.
 *
 * @param out_node output node id
 * @param arg check callback function argument
 * @param cb check callback function
 *
 * @return true means register callback function ok,otherwise is false
 */
bool_t mem_monitor_register_node(uint8_t *out_node, void* arg, node_check_callback cb);

/**
 * @brief This function is to clear a memory detect node.
 *
 * @param node clear input detect node
 *
 * @return true means clear callback function ok,otherwise is false
 */
bool_t mem_monitor_clear_node(uint8_t node);

/**
 * @brief This function is to clear all memory detect node.
 *
 * @return true means clear callback function ok,otherwise is false
 */
void mem_monitor_clear_all(void);

#endif
