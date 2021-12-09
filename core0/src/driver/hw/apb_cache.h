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
#ifndef _DRIVER_HW_CACHE_H_
#define _DRIVER_HW_CACHE_H_
#include "types.h"
#include "apb.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APB_CACHE_SFC_BASE  0x04000000
#define APB_CACHE_SMC_BASE  0x24000000
#define APB_CACHE_MINI_BASE 0x21000000
#define APB_CACHE_SFC_ID    0
#define APB_CACHE_SMC_ID    1
#define APB_CACHE_MINI_ID   2

void apb_cache_reset(uint8_t cache_id);
void apb_cache_enable(uint8_t cache_id);
void apb_cache_enable_ram(uint8_t cache_id);
void apb_cache_disable(uint8_t cache_id);
void apb_cache_invalidate(uint8_t cache_id, uint32_t addr, uint32_t len);
void apb_cache_flush(uint8_t cache_id, uint32_t addr, uint32_t len);
void apb_cache_space_ena(uint8_t cache_id);
void apb_cache_space_dis(uint8_t cache_id);
void apb_cache_clear(uint8_t cache_id);
void apb_cache_init(uint8_t cache_id);
void apb_cache_deinit(uint8_t cache_id);
uint32_t apb_cache_miss_cnt(uint8_t cache_id);
void apb_cache_miss_clear(uint8_t cache_id);
void apb_cache_miss_enable(uint8_t cache_id, bool_t en);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_CACHE_H_ */
