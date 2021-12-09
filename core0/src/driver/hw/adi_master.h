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

#ifndef DRIVER_HW_ADI_MASTER_H
#define DRIVER_HW_ADI_MASTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "adi_common.h"

void adi_master_audio_channel_sync(ADI_WIRE_MODE wire_mode);
void adi_master_audio_sync(void);
void adi_master_fifo_depth(uint8_t depth);
void adi_master_ddr_mode(bool_t ddr_mode);
void adi_master_bist_set_vector(const uint32_t *vector);
void adi_master_bist_start(void);
uint32_t adi_master_bist_get_error_count(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_HW_ADI_MASTER_H */
