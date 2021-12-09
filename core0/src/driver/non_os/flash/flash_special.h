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
#ifndef _DRIVER_NON_OS_FLASH_SPECIAL_H
#define _DRIVER_NON_OS_FLASH_SPECIAL_H

#ifdef __cplusplus
extern "C" {
#endif

#define P25Q40L_ID                0x1285
#define P25Q80LE_ID               0x1385
#define P25Q16LE_ID               0x1485 //puya 2MB ID
#define P25Q32LE_ID               0x1585

#define P25Q16LE_SFDP             0xF1 //PUYA 2MB 55nm SFDP
#define P25Q16SL_SFDP             0xF9 //PUYA 2MB 40nm SFDP
#define CFG_QP_BIT_S4             0x10

typedef enum {
    QE_ON_STS1 = 1,
    QE_ON_STS2,
} flash_special_qe_position_t;

struct flash_ctrl {
    void (*set_quad_mode)(void);
    void (*enable_qpp_mode)(void);
    void (*disable_qpp_mode)(void);
    void (*otp_lock)(uint8_t id);
    uint8_t (*page_program)(uint32_t addr, const void *buf, size_t count);
    uint8_t (*sector_erase)(uint32_t addr);
    uint8_t (*otp_write)(uint32_t addr, const void *buf, size_t count);
    uint8_t (*otp_erase)(uint32_t addr);
    uint8_t (*chip_erase)(void);
    uint8_t (*page_program_sw)(uint32_t addr, const void *buf, size_t count);
    uint8_t (*sector_erase_sw)(uint32_t addr);
    uint8_t (*sw_pe_alg)(void);
    void (*read_info)(void);
};

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_NON_OS_FLASH_SPECIAL_H */
