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
#ifndef _DRIVER_NON_OS_FLASH_COMMON_H
#define _DRIVER_NON_OS_FLASH_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "flash_special.h"

#define FLASH_PAGE_SIZE  0x100U
#define FLASH_QPAGE_SIZE 0x400U

#define param_a(a, b, c, d, e) a
#define param_b(a, b, c, d, e) b
#define param_c(a, b, c, d, e) c
#define param_d(a, b, c, d, e) d
#define param_e(a, b, c, d, e) e

#define CMD(x)       param_a(x)
#define OP_CODE(x)   param_b(x)
#define SWM_MODE(x)  param_c(x)
#define SWM_CYCLE(x) param_d(x)
#define WTIME(x)     param_e(x)
#define FLASH_CMD(x)                                                      \
    {                                                                     \
        .cmd = param_a(x), .op_mode = param_b(x), .swm_mode = param_c(x), \
        .swm_cycle = param_d(x), .wtime = param_e(x)                      \
    }

/**                       cmd   opmode        swm_mode, swm_cycle, wtime */
#define READ_ID           0x9F, SFC_OP_REG_WR, 0x100,   0x08180000, 0x2000
#define WRITE_EN          0x06, SFC_OP_REG_WR, 0x200,   0x08000000, 0x2000
#define WRITE_DIS         0x04, SFC_OP_REG_WR, 0x200,   0x08000000, 0x2000
#define READ_STS_REG1     0x05, SFC_OP_REG_WR, 0x100,   0x08000000, 0x2000
#define READ_STS_REG2     0x35, SFC_OP_REG_WR, 0x100,   0x08000000, 0x2000
#define MANU_DEV_ID       0x90, SFC_OP_REG_WR, 0x100,   0x08180000, 0x2000
#define READ_DATA         0x03, SFC_OP_TRANS,  0x100,   0x08180000, 0x2000
#define QUAD_READ_DATA    0xEB, SFC_OP_TRANS,  0x115,   0x08060204, 0x2000
#define RESET_EN          0x66, SFC_OP_REG_WR, 0x200,   0x08000000, 0x2000
#define RESET             0x99, SFC_OP_REG_WR, 0x200,   0x08000000, 0x2000
#define OTP_READ_DATA     0x48, SFC_OP_TRANS,  0x100,   0x08180008, 0x2000
#define RDSFDP_ID         0x5A, SFC_OP_TRANS,  0x100,   0x08180008, 0x2000
#define RD_UNIQ_ID        0x4B, SFC_OP_TRANS,  0x100,   0x08180008, 0x2000
#define PE_SUSPEND        0x75, SFC_OP_REG_WR, 0x200,   0x08000000, 0x2000
#define PE_RESUME         0x7A, SFC_OP_REG_WR, 0x200,   0x08000000, 0x2000
#define READ_CFG_REG      0x15, SFC_OP_REG_WR, 0x100,   0x08000000, 0x2000
#define WRITE_CFG_REG     0x11, SFC_OP_REG_WR, 0x200,   0x08000000, 0x2000

#define WRITE_STS_REG1    0x01, SFC_OP_REG_WR, 0x200, 0x08000000, 0x2000
#define WRITE_STS_REG2    0x31, SFC_OP_REG_WR, 0x200, 0x08000000, 0x2000

#define STS_WIP_BIT_S0        0x01
#define STS_WEL_BIT_S1        0x02
#define STS_QE_BIT_S9         0x02
#define STS_LB1_BIT_S11       0x08
#define STS_LB2_BIT_S12       0x10
#define STS_LB3_BIT_S13       0x20
#define STS_SUS2_BIT_S10      0x04
#define STS_SUS1_BIT_S15      0x80

#define PUYA_MANU_ID          0x85
#define GIGA_MANU_ID          0xc8
#define FLASH_UNIQUE_ID_LEN   0x10

#define FLASH_OTP_EXTERNAL         0xFFFFCFFF
#define FLASH_OTP_REGIN_MASK       0x3000
#define FLASH_OTP_REGIN_VALID_BYTE 0x1ff

#define FLASH_SFDP_INTERNAL_ADDRESS    0x32

/** @brief flash otp region id.*/
typedef enum {
    FLASH_OTP_REGION0 = 1,
    FLASH_OTP_REGION1,
    FLASH_OTP_REGION2,
    FLASH_OTP_REGION_MAX,
} FLASH_OTP_REGION_ID;

typedef struct flash_id_info {
    uint8_t v_flag;
    uint8_t param_offset;
    uint32_t uid;
} flash_id_info_t;

#define FLASH_PE_INFO(rw, sw, pecw, pacw, wgwt, ppt, set) \
{                                  \
    .resume_wait_time = rw,        \
    .sus_wait_time = sw,           \
    .pe_cmd_wait_time = pecw,      \
    .pause_wait_time = pacw,       \
    .wip_gap_wait_time = wgwt,     \
    .page_program_time = ppt,      \
    .sector_erase_time = set,      \
}

typedef enum{
    FLASH_PE_HW_MODE,
    FLASH_PE_SW_MODE,
}FLASH_PE_MODE;

/**
 * @brief This function is to check WIP status.
 *
 * @return bool_t is true when WIP is 1.
 */
bool_t flash_special_check_wip_status(void);

/**
 * @brief This function is to check flash status and waiting in process.
 *
 */
void flash_special_wait_wip(void);

/**
 * @brief This function is to set flash write enable.
 *
 */
void flash_special_write_enable(void);

/**
 * @brief This function is to set flash write disable.
 *
 */
void flash_special_write_disable(void);

/**
 * @brief This function is to write reg1 status into flash.
 *
 * @param data is data to write into flash.
 * @param len is data len
 */
void flash_special_write_status_reg1(uint32_t data, uint8_t len);

/**
 * @brief This function is to write reg2 status into flash.
 *
 * @param data data is data to write into flash.
 * @param len len is data len
 */
void flash_special_write_status_reg2(uint32_t data, uint8_t len);

/**
 * @brief This function is to read reg1 status from flash.
 *
 * @return uint8_t reg1 status.
 */
uint8_t flash_special_read_status_reg1(void);

/**
 * @brief This function is to read reg2 status from flash.
 *
 * @return uint8_t reg2 status.
 */
uint8_t flash_special_read_status_reg2(void);

/**
 * @brief This function is to read data from flash.
 *
 * @param addr is flash addr
 * @param buf is buffer ready write to flash
 * @param count is buffer len
 * @return uint8_t RET_OK or RET_INVAL,RET_OK for success else error..
 */
uint8_t flash_special_read(uint32_t addr, void *buf, size_t count);

/**
 * @brief This function is to erase flash sector according to sector id.
 *
 * @param sector_id is flash sector id
 * @return uint8_t RET_INVAL or RET_OK,RET_OK for success else error..
 */
uint8_t flash_special_erase_sector(uint16_t sector_id);

/**
 * @brief This function is to erase flash according to addr.
 *
 * @param addr is flash addr
 * @return uint8_t RET_INVAL or RET_OK,RET_OK for success else error..
 */
uint8_t flash_special_erase(uint32_t addr);

/**
 * @brief This function is to write data into flash.
 *
 * @param addr is flash addr
 * @param buf is buffer ready write to flash
 * @param count is buf len
 * @return uint8_t RET_INVAL or RET_OK,RET_OK for success else error..
 */
uint8_t flash_special_write(uint32_t addr, const void *buf, size_t count);

/**
 * @brief This function is to get flash id.
 *
 * @return uint16_t flash id.
 */
uint16_t flash_special_get_id(void);

/**
 * @brief This function is to check if flash is quad mode.
 *
 * @return bool_t true or false.
 */
bool_t flash_special_is_quad_mode(void);

/**
 * @brief This function is to enable flash quad mode.
 *
 */
void flash_special_enable_quad_mode(void);

/**
 * @brief This function is to disable flash quad mode.
 *
 */
void flash_special_disable_quad_mode(void);

/**
 * @brief This function is to erase flash through otp certification .
 *
 * @param addr is flash addr that ready to erase
 * @return uint8_t RET_INVAL or RET_OK, RET_OK for success else error..
 */
uint8_t flash_special_otp_erase(uint32_t addr);

/**
 * @brief This function is to write data into flash through otp certification.
 *
 * @param addr is flash addr
 * @param buf is buffer ready write to flash
 * @param count is buf len
 * @return uint8_t RET_INVAL or RET_OK,RET_OK for success else error..
 */
uint8_t flash_special_otp_write(uint32_t addr, void *buf, size_t count);

/**
 * @brief This function is to read data from flash through otp certification.
 *
 * @param addr is flash addr
 * @param buf is buffer for the data being read
 * @param count is buffer len
 * @return uint8_t RET_INVAL or RET_OK,RET_OK for success else error..
 */
uint8_t flash_special_otp_read(uint32_t addr, void *buf, size_t count);

/**
 * @brief This function is to set flash cache mode.
 *
 */
void flash_special_set_cache_mode(void);

/**
 * @brief This function is to install flash (puya or gd) special operation.
 *
 */
void flash_init(void);

/**
 * @brief This function is to enable flash qpp mode.
 *
 */
void flash_special_enable_qpp_mode(void);

/**
 * @brief This function is to disable flash qpp mode.
 *
 */
void flash_special_disable_qpp_mode(void);

/**
 * @brief This function is to lock the flash otp region.
 *
 * @param id is flash otp region number.
 */
void flash_special_otp_lock(uint8_t id);

/**
 * @brief This function is read the flash config register.
 *
 */
uint8_t flash_special_read_config_reg(void);

/**
 * @brief This function is write the flash cfg to config register.
 *
 * @param data is flash register configure.
 */
void flash_special_write_config_reg(uint8_t data);

/**
 * @brief This function is read Serial Flash Discoverable Parameter(SFDP).
 *
 * @return uint8_t SFDP
 */
uint8_t flash_special_get_sfdp(void);

/**
 * @brief This function is to chip erase flash.
 *
 * @return uint8_t RET_OK
 */
uint8_t flash_special_chip_erase(void);

/**
 * @brief This function is to run P/E suspend command.
 *
 */
void flash_special_pe_suspend(void);

/**
 * @brief This function is to run P/E resume command.
 *
 */
void flash_special_pe_resume(void);

/**
 * @brief This function is to soft reset flash. Controller will send reset_en and reset command.
 *
 */
void flash_special_soft_reset(void);

/**
 * @brief This function is to set WIP's wait time when program/erase.
 *
 * @param p_time is WIP wait time when program, unit is us.
 * @param e_time is WIP wait time when erase, unit is us.
 */
void flash_special_set_wip_wait_time(uint32_t p_time, uint32_t e_time);

/**
 * @brief This function is to get WIP'S wait time when program. When return zero, use default value.
 *
 * @return uint32_t is WIP's wait time when program, unit is us.
 */
uint32_t flash_special_get_program_wip_wait_time(void);

/**
 * @brief This function is to get WIP'S wait time when erase. When return zero, use default value.
 *
 * @return uint32_t is WIP's wait time when erase, unit is us.
 */
uint32_t flash_special_get_erase_wip_wait_time(void);

/**
 * @brief This function is to get wheter program/erase command is running.
 *
 * @return bool_t is true if program/erase command is running.
 */
bool_t flash_special_is_pe_in_progress(void);

/**
 * @brief This function is to set program/erase mode whether it use SFC's state machine.
 *
 * @param mode is FLASH_PE_SW_MODE or FLASH_PE_HW_MODE.
 */
void flash_special_set_pe_mode(FLASH_PE_MODE mode);

/**
 * @brief This function is to get program/erase mode.
 *
 * @return FLASH_PE_MODE is FLASH_PE_SW_MODE or FLASH_PE_HW_MODE.
 */
FLASH_PE_MODE flash_special_get_pe_mode(void);

uint8_t flash_special_get_uid(void *data);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_NON_OS_FLASH_COMMON_H */
