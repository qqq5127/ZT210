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
#ifndef FRAGMENTARY_KV__H_
#define FRAGMENTARY_KV__H_

#include "kv_id.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PPM_ADJUST_ID = FRAGMENTARY_BASE_ID,
} FRAGMENTARY_KV_ID;

typedef struct {
    int8_t ppm_adjust;
} ppm_adjust_kv_cfg_t;

#ifdef __cplusplus
}
#endif

#endif /* FRAGMENTARY_KV__H_ */
