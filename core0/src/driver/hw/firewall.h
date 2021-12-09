/****************************************************************************

Copyright(c) 2021 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
#ifndef _HW_FIREWALL_H_
#define _HW_FIREWALL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FIREWALL_0 = 0,
    FIREWALL_1,
    FIREWALL_2,
    FIREWALL_MAX_NUM,
} FIREWALL_ID;

typedef enum {
    FIREWALL_RO = 0,
    FIREWALL_WO,
    FIREWALL_WR,
} FIREWALL_WR_ID;

typedef enum {
    FIREWALL_SEGMENT_0 = 0,
    FIREWALL_SEGMENT_1,
    FIREWALL_SEGMENT_2,
    FIREWALL_SEGMENT_3,
    FIREWALL_SEGMENT_MAX_NUM,
} FIREWALL_SEGMENT_ID;

typedef enum {
    FIREWALL_MASTER_RV0_I = 0,
    FIREWALL_MASTER_RV0_D,
    FIREWALL_MASTER_DSP_S1,
    FIREWALL_MASTER_BT_S7,
    FIREWALL_MASTER_SW_DMA0_I,
    FIREWALL_MASTER_SW_DMA0_D,
    FIREWALL_MASTER_FDMA,
    FIREWALL_MASTER_7,
    FIREWALL_MASTER_MAX_NUM,
} FIREWALL_MASTER_ID;

void firewall_init(uint8_t id);
void firewall_deinit(uint8_t id);
uint8_t firewall_segment_get_num(void);
bool_t firewall_slaver_config(uint8_t id, uint8_t enable, uint8_t master, uint32_t addr);
uint8_t firewall_master_config(uint8_t id, FIREWALL_WR_ID wr, uint32_t start, uint32_t end);

#ifdef __cplusplus
}
#endif

#endif /* _HW_FIREWALL_H_ */
