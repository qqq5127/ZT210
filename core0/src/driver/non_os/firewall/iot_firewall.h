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
#ifndef _IOT_FIREWALL_TEST_H
#define _IOT_FIREWALL_TEST_H

typedef enum {
    IOT_FIREWALL_0 = 0,
    IOT_FIREWALL_1,
    IOT_FIREWALL_2,
    IOT_FIREWALL_MAX_NUM,
} IOT_FIREWALL_ID;

typedef enum {
    IOT_FIREWALL_RO = 0,
    IOT_FIREWALL_WO,
    IOT_FIREWALL_WR,
} IOT_FIREWALL_WR_ID;

typedef enum {
    IOT_FIREWALL_MASTER_RV0_I = 0,
    IOT_FIREWALL_MASTER_RV0_D,
    IOT_FIREWALL_MASTER_DSP_S1,
    IOT_FIREWALL_MASTER_BT_S7,
    IOT_FIREWALL_MASTER_SW_DMA0_I,
    IOT_FIREWALL_MASTER_SW_DMA0_D,
    IOT_FIREWALL_MASTER_FDMA,
    IOT_FIREWALL_MASTER_7,
    IOT_FIREWALL_MASTER_MAX_NUM,
} IOT_FIREWALL_MASTER_ID;

#define IOT_FIREWALL_SLAVE_MAX_NUM 10

void iot_firewall_init(void);
void iot_firewall_deinit(void);
uint8_t iot_firewall_segment_get_num(void);
bool_t iot_firewall_slaver_config(uint8_t enable, uint8_t master, uint32_t addr);
uint8_t iot_firewall_master_config(IOT_FIREWALL_WR_ID wr, uint32_t start, uint32_t end);

#endif
