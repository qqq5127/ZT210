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
#ifndef DATA_DUMP_H
#define DATA_DUMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

/**
 * @brief dump a piece of data
 *
 * @param data the data to dump
 * @param len length of the data
 */
void dump_bytes(const uint8_t *data, int len);

#endif
