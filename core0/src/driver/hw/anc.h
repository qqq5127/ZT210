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
#ifndef _DRIVER_HW_ANC_H
#define _DRIVER_HW_ANC_H

#include "types.h"

typedef enum {
    ANC_COEFF_PARAM_0,
    ANC_COEFF_PARAM_1,
    ANC_COEFF_PARAM_MAX,
} ANC_COEFF_PARAM_SET;

typedef enum {
    ANC_CLIPPING_0,
    ANC_CLIPPING_1,
    ANC_CLIPPING_2,
    ANC_CLIPPING_3,
    ANC_CLIPPING_MAX
} ANC_CLIPPING_ID;

typedef enum {
    ANC_GATING_0,
    ANC_GATING_1,
    ANC_GATING_2,
    ANC_GATING_3,
    ANC_GATING_MAX
} ANC_GATING_ID;

typedef enum {
    ANC_BIQUAD_0,
    ANC_BIQUAD_1,
    ANC_BIQUAD_2,
    ANC_BIQUAD_3,
    ANC_BIQUAD_4,
    ANC_BIQUAD_5,
    ANC_BIQUAD_6,
    ANC_BIQUAD_7,
    ANC_CLIPPING0,
    ANC_CLIPPING1,
    ANC_CLIPPING2,
    ANC_CLIPPING3,
    ANC_GATING0,
    ANC_GATING1,
    ANC_GATING2,
    ANC_GATING3,
    ANC_ID_MAX,
} ANC_COMPONENT_ID;

typedef enum {
    ANC_IN_SRC_DFE_0,
    ANC_IN_SRC_DFE_1,
    ANC_IN_SRC_DFE_2,
    ANC_IN_SRC_DFE_3,
    ANC_IN_SRC_DFE_4,
    ANC_IN_SRC_DFE_5,
    ANC_IN_SRC_BIQUAD_0,
    ANC_IN_SRC_BIQUAD_1,
    ANC_IN_SRC_BIQUAD_2,
    ANC_IN_SRC_BIQUAD_3,
    ANC_IN_SRC_BIQUAD_4,
    ANC_IN_SRC_BIQUAD_5,
    ANC_IN_SRC_BIQUAD_6,
    ANC_IN_SRC_BIQUAD_7,
    ANC_IN_SRC_CLIPPING_0,
    ANC_IN_SRC_CLIPPING_1,
    ANC_IN_SRC_CLIPPING_2,
    ANC_IN_SRC_CLIPPING_3,
    ANC_IN_SRC_GATING_0,
    ANC_IN_SRC_GATING_1,
    ANC_IN_SRC_GATING_2,
    ANC_IN_SRC_GATING_3,
    ANC_IN_SRC_ASRC_0,
    ANC_IN_SRC_ASRC_1,
    ANC_IN_SRC_MAX,
    ANC_IN_SRC_NONE = 0x1f
} ANC_IN_SRC_ID;

typedef enum {
    ANC_OUT_DFE_0,
    ANC_OUT_DFE_1,
    ANC_OUT_MAX,
} ANC_OUT_ID;

bool_t anc_get_it_raw_status(void);
bool_t anc_get_it_status(void);
void anc_it_clear(bool_t clr);
void anc_it_enable(bool_t en);

void anc_enable(bool_t enable);
void anc_frame_div_counter(uint8_t frame_div_cnt);
void anc_sample_rate(uint32_t rate, uint8_t frame_cnt);
void anc_out_bypass_asrc(ANC_OUT_ID id, bool_t enable);
void anc_sw_coeff_access_enable(bool_t enable);
void anc_sw_coeff_select(ANC_COEFF_PARAM_SET set);
void anc_in_link(ANC_COMPONENT_ID id, ANC_IN_SRC_ID in);
void anc_clip_rate(ANC_CLIPPING_ID id, uint8_t rate);
void anc_clipping_thres(ANC_CLIPPING_ID id, uint32_t thres);
void anc_gating_duration(ANC_GATING_ID id, uint16_t duration);
void anc_gating_thres(ANC_GATING_ID id, uint32_t thres0, uint32_t thres1);
void anc_out_link(ANC_COMPONENT_ID id, ANC_OUT_ID out);
void anc_out_link_delect(ANC_COMPONENT_ID id, ANC_OUT_ID out);
void anc_out_clipping_thres(ANC_OUT_ID id, uint32_t thres);
void anc_out_clipping_rate(ANC_OUT_ID id, uint8_t rate);
void anc_out_gating_duration(ANC_OUT_ID id, uint16_t duration);
void anc_out_gating_thres(ANC_OUT_ID id, uint32_t thres0, uint32_t thres1);
uint8_t anc_write_coeff_param(ANC_COEFF_PARAM_SET set, uint32_t offset,
                              const uint32_t *param, uint32_t len);
uint8_t anc_get_coeff_select(void);

#endif
