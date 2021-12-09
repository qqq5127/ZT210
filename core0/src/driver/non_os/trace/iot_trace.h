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

#ifndef LIB_TRACE_H
#define LIB_TRACE_H

#include "types.h"

enum IOT_TRACE_MATCH_OPTION {
    IOT_TRACE_MATCH_EQUAL,
#ifdef IOT_TRACE_NOTEQUAL
    IOT_TRACE_MATCH_GREAT_EQUAL,  // beetle not support this option
    IOT_TRACE_MATCH_LESS,         // beetle not support this option
#endif
};

/**
 * @brief This function is to watch detect.
 *
 * @param addr is the address of trigger.
 * @return int RET_OK for success else error.
 */
int iot_trigger_watch_detect(void *addr);

/**
 * @brief This function is to watch value detect.
 *
 * @param addr is the address of trigger.
 * @param val is the value.
 * @return int RET_OK for success else error.
 */
#define iot_trigger_watch_val_detect(addr, val) iot_trigger_watch_val_range(addr, val, IOT_TRACE_MATCH_EQUAL)

/**
 * @brief This function is to watch value detect.
 *
 * @param addr is the address of trigger.
 * @param val is the value.
 * @param option is the match option.
 *
 * @return int RET_OK for success else error.
 */
int iot_trigger_watch_val_range(void *addr, uint32_t val, enum IOT_TRACE_MATCH_OPTION option);

#endif /*LIB_TRACE_H*/
