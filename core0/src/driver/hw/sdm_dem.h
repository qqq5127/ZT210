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

#ifndef __DRIVER_HW_SDM_DEM_H__
#define __DRIVER_HW_SDM_DEM_H__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SDM_DEM_CHN_0,
    SDM_DEM_CHN_1,
    SDM_DEM_CHN_DOUBLE,
    SDM_DEM_CHN_MAX,
} SDM_DEM_CHN_ID;

void sdm_dem_reset(void);
void sdm_dem_soft_reset(SDM_DEM_CHN_ID chn);
void sdm_dem_enable(SDM_DEM_CHN_ID chn, bool_t enable);
void sdm_dem_set_overwrite_data(SDM_DEM_CHN_ID chn, bool_t set);
void sdm_dem_overwrite_enable(SDM_DEM_CHN_ID chn, bool_t enable);
void sdm_dem_data_format(SDM_DEM_CHN_ID chn, bool_t unsigned_format);
void sdm_dem_bit_invert(SDM_DEM_CHN_ID chn, bool_t invert);
void sdm_dem_barrel_shift_operate(SDM_DEM_CHN_ID chn, bool_t enable, uint8_t opt);
void sdm_dem_lfsr_overwrite_en(bool_t enable);
void sdm_dem_lfsr_init_code(uint16_t init_code);
void sdm_dem_init_code_hight_11bit_overwrite(uint16_t hight_bit);

#ifdef __cplusplus
}
#endif

#endif
