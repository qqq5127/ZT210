
/****************************************************************************

Copyright(c) 2019 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
#ifndef _IOT_CORE_DUMP_H
#define _IOT_CORE_DUMP_H

#include "types.h"
#include "stdio.h"
#include "riscv_cpu.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is call back function type define
 *
 * @return RET_OK or RET_FAIL.
 */
typedef int32_t (*iot_core_dump_callback)(const exception_info_t *info);

/**
 * @brief This function is used to write core dump(only task) information to uart.
 *
 * @return RET_OK or RET_FAIL.
 */
int32_t iot_task_dump_to_uart(const exception_info_t *info);

/**
 * @brief   iot_core_dump_init - init coredump module.
 */
void iot_core_dump_init(void);

#ifdef __cplusplus
}
#endif

#endif   //_IOT_CORE_DUMP_H
