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
#ifndef KEY_VALUE_CACHE_H
#define KEY_VALUE_CACHE_H

#include "key_value.h"

#ifdef __cplusplus
extern "C" {
#endif

KV_ERROR key_value_cache_get_key(uint16_t id, uint8_t **data, uint32_t *length);
KV_ERROR key_value_cache_add_key(uint16_t id, const uint8_t *data, uint32_t length);
KV_ERROR key_value_cache_write_key(uint16_t id, const uint8_t *data, uint32_t length, bool_t writeable);
KV_ERROR key_value_cache_invalid(uint16_t id);
KV_ERROR key_value_cache_flush(void);
KV_ERROR key_value_cache_init(void);

#ifdef __cplusplus
}
#endif

#endif /* KEY_VALUE_CACHE_H */
