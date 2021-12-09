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
#ifndef _DRIVER_NON_OS_DDR_AP_H
#define _DRIVER_NON_OS_DDR_AP_H
#include "types.h"

#define REG_WR_INST_AP_32M 0xC000
#define REG_RD_INST_AP_32M 0x4000
#define REG_WR_INST_AP_64M 0xC0C0
#define REG_RD_INST_AP_64M 0x4040

#define DATA_WR_INST_AP_32M 0x8000
#define DATA_RD_INST_AP_32M 0x0000
#define DATA_WR_INST_AP_64M 0xA0A0
#define DATA_RD_INST_AP_64M 0x2020

#define DDR_SPECIAL_AP_32M_LATENCY 1
#define DDR_SPECIAL_AP_64M_LATENCY 3
#define DDR_SPECIAL_AP_64M_WRADDR1 0
#define DDR_SPECIAL_AP_64M_WRADDR2 4
#define DDR_SPECIAL_AP_32M_WRADDR  0

/*DDR 4M byte size*/
#define AP_DDR_SPECIAL_4MB_SIZE 0x400000

/*DDR 8M byte size*/
#define AP_DDR_SPECIAL_8MB_SIZE 0x800000

/**
 * @brief This function is to init DDR in special style.
 *
 */
void ddr_special_init(void);

#endif /* _DRIVER_NON_OS_DDR_AP_H */
