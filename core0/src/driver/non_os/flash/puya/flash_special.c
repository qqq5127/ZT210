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
#include "types.h"
#include "riscv_cpu.h"
#include "driver_dbglog.h"

#include "flash.h"
#include "sfc.h"

#include "iot_timer.h"
#include "iot_soc.h"

/**                            cmd   opmode        swm_mode, swm_cycle, wtime */
#define PUYA_PAGE_PROGRAM      0x02, SFC_OP_PRM,   0x200, 0x08180000, 3000     //page program=3ms
#define PUYA_QUAD_PAGE_PROGRAM 0x32, SFC_OP_PRM,   0x201, 0x08180000, 3000   //quad page program=3ms
#define PUYA_SECTOR_ERASE      0x20, SFC_OP_ERASE, 0x200, 0x08180000, 30000  //sector erase time=30ms
#define PUYA_OTP_ERASE         0x44, SFC_OP_ERASE, 0x200, 0x08180000, 8000   //otp erase time=8ms
#define PUYA_OTP_PAGE_PROGRAM  0x42, SFC_OP_PRM,   0x200, 0x08180000, 8000   //otp program time=8ms
#define PUYA_CHIP_ERASE        0x60, SFC_OP_ERASE, 0x200, 0x08000000, 180000 //chip erase time=180ms

const uint32_t flash_puya_uid[] IRAM_RODATA(flash_puya_uid) = {
    0x1485F941, //2MB 40nm SLA
    0x1485F942, //2MB 40nm SLC
    0x1485F141, //2MB 55nm
    0x1585F942, //4MB SLA
};

static const sfc_pe_param_t puya_pe_param[] = {
    FLASH_PE_INFO(200, 100, 200, 500, 64, 4000, 30000),   //2MB 40nm SLA
    FLASH_PE_INFO(200, 100, 200, 500, 64, 4000, 30000),   //2MB 40nm SLC
    FLASH_PE_INFO(200, 100, 200, 500, 64, 4000, 30000),   //2MB 55nm
    FLASH_PE_INFO(200, 100, 200, 500, 64, 4000, 30000)    //4MB SLA
};

static flash_id_info_t flash_puya_info = {
    0,      //initial state flag: invalid
    0,      //invalid
    0       //invalid flash uid
};

typedef struct flash_special_sts_reg_wr {
    uint32_t flash_id;
    uint8_t write_sts2_reg_len;
} flash_special_sts_reg_wr_t;

static const flash_special_sts_reg_wr_t flash_special_sts_wr_list[] IRAM_RODATA(flash_special_sts_wr_list) = {
    {P25Q80LE_ID, QE_ON_STS2},
    {P25Q40L_ID, QE_ON_STS2},
    {P25Q32LE_ID, QE_ON_STS1},
};

static uint8_t flash_special_get_wr_sts2_len(void) IRAM_TEXT(flash_special_get_wr_sts2_len);
static uint8_t flash_special_get_wr_sts2_len(void)
{
    uint8_t i = 0;
    uint8_t sfdp = 0;
    uint16_t id;

    id = flash_special_get_id();

    if (id == P25Q16LE_ID) {
        //puya 2MB flash must be distinguished by sfdp.
        sfdp = flash_special_get_sfdp();

        if (sfdp == P25Q16LE_SFDP) {
            return QE_ON_STS2;
        } else if (sfdp == P25Q16SL_SFDP) {
            return QE_ON_STS1;
        }
    } else {
        for (i = 0; i < ARRAY_SIZE(flash_special_sts_wr_list); i++) {
            if (id == flash_special_sts_wr_list[i].flash_id) {
                return flash_special_sts_wr_list[i].write_sts2_reg_len;
            }
        }
        assert(0);
    }

    return 0;
}

static void flash_puya_set_quad_mode(void) IRAM_TEXT(flash_puya_set_quad_mode);
static void flash_puya_set_quad_mode(void)
{
    uint8_t len;
    uint8_t sts1 = 0;
    uint8_t sts2;
    uint16_t sts = 0;

    sts2 = flash_special_read_status_reg2();
    if (sts2 & STS_QE_BIT_S9) {
        return;
    }

    len = flash_special_get_wr_sts2_len();

    if (len == QE_ON_STS2) {
        sts2 |= STS_QE_BIT_S9;
        sts1 = flash_special_read_status_reg1();
        sts = sts1 | (sts2 << 8);
        flash_special_write_enable();

        flash_special_write_status_reg1(sts, 2);
    } else if (len == QE_ON_STS1) {
        sts2 |= STS_QE_BIT_S9;

        flash_special_write_enable();

        flash_special_write_status_reg2(sts2, 1);
    }

    flash_special_wait_wip();

    while (!(flash_special_read_status_reg2() & STS_QE_BIT_S9)) {
    }
}

static void flash_puya_enable_qpp_mode(void)
{
    uint8_t data;

    data = flash_special_read_config_reg();
    if (data & CFG_QP_BIT_S4) {
        return;
    }

    data |= CFG_QP_BIT_S4;

    flash_special_write_enable();

    flash_special_write_config_reg(data);

    flash_special_wait_wip();
    while (!(flash_special_read_status_reg2() & CFG_QP_BIT_S4)) {
    }
}

static void flash_puya_disable_qpp_mode(void)
{
}

static void flash_puya_otp_lock(uint8_t id)
{
    uint8_t sts1 = 0;
    uint8_t sts2;
    uint8_t len;
    uint8_t mask = 0;
    uint16_t sts = 0;

    switch (id) {
        case FLASH_OTP_REGION0:
            mask = STS_LB1_BIT_S11;
            break;
        case FLASH_OTP_REGION1:
            mask = STS_LB2_BIT_S12;
            break;
        case FLASH_OTP_REGION2:
            mask = STS_LB3_BIT_S13;
            break;
        default:
            return;
    }

    sts2 = flash_special_read_status_reg2();

    if (sts2 & mask) {
        return;
    }
    len = flash_special_get_wr_sts2_len();

    if (len == QE_ON_STS2) {
        sts2 |= mask;
        sts1 = flash_special_read_status_reg1();
        sts = sts1 | (sts2 << 8);
        flash_special_write_enable();
        flash_special_write_status_reg1(sts, 2);
    } else if (len == QE_ON_STS1) {
        sts2 |= mask;

        flash_special_write_enable();
        flash_special_write_status_reg2(sts2, 1);
    }

    flash_special_wait_wip();

    while (!(flash_special_read_status_reg2() & mask)) {
    }
}

void flash_puya_soft_reset(void) IRAM_TEXT(flash_puya_soft_reset);
void flash_puya_soft_reset(void)
{
    uint32_t mask = cpu_disable_irq();

    sfc_send_soft_reset_cmd_with_pe_sm();

    cpu_restore_irq(mask);
}

//pe workaround only apply to puya 2MB 40nm SLA series.
void flash_puya_pe_war(void) IRAM_TEXT(flash_puya_pe_war);
void flash_puya_pe_war(void)
{
    if (flash_puya_info.uid == 0x1485F941) {
        flash_puya_soft_reset();
    }
}

static uint8_t flash_puya_page_program(uint32_t addr, const void *buf,
                                          size_t count)
{
    sfc_cmd_t pp = FLASH_CMD(PUYA_PAGE_PROGRAM);
    sfc_cmd_t qpp = FLASH_CMD(PUYA_QUAD_PAGE_PROGRAM);
    sfc_cmd_t *cmd = NULL;
    uint8_t ret;

    uint32_t wtime = flash_special_get_program_wip_wait_time();

    if (flash_special_is_quad_mode()) {
        cmd = &qpp;
    } else {
        cmd = &pp;
    }

    cmd->addr = addr;

    /* check WIP status */
    flash_special_wait_wip();

    uint8_t offset = flash_puya_info.param_offset;

    cmd->wtime = puya_pe_param[offset].page_program_time;
    if (wtime != 0) {
        cmd->wtime = wtime;
    }

    /* write enable cmd*/
    flash_special_write_enable();

    sfc_set_dynamic_suspend_resume_param(&puya_pe_param[offset], cmd->wtime);

    ret = sfc_send_write_cmd(cmd, buf, count);

    flash_puya_pe_war();

    return ret;
}

static uint8_t flash_puya_erase(uint32_t addr)
{
    sfc_cmd_t cmd = FLASH_CMD(PUYA_SECTOR_ERASE);
    uint8_t ret;

    uint32_t wtime = flash_special_get_erase_wip_wait_time();

    cmd.addr = addr;

    /* check WIP status */
    flash_special_wait_wip();

    uint8_t offset = flash_puya_info.param_offset;

    cmd.wtime = puya_pe_param[offset].sector_erase_time;

    if (wtime != 0) {
        cmd.wtime = wtime;
    }

    flash_special_write_enable();

    sfc_set_dynamic_suspend_resume_param(&puya_pe_param[offset], cmd.wtime);
    ret = sfc_send_cmd(&cmd);

    flash_puya_pe_war();

    return ret;
}

static uint8_t flash_puya_otp_page_program(uint32_t addr, const void *buf, size_t count)
{
    sfc_cmd_t cmd = FLASH_CMD(PUYA_OTP_PAGE_PROGRAM);
    uint8_t ret;

    cmd.addr = addr;

    /* check WIP status */
    flash_special_wait_wip();

    /* write enable cmd*/
    flash_special_write_enable();

    ret = sfc_send_write_cmd(&cmd, buf, count);

    return ret;
}

static uint8_t flash_puya_otp_erase(uint32_t addr)
{
    uint8_t ret;
    sfc_cmd_t cmd = FLASH_CMD(PUYA_OTP_ERASE);

    cmd.addr = addr;

    flash_special_wait_wip();

    flash_special_write_enable();
    ret = sfc_send_cmd(&cmd);

    return ret;
}

static uint8_t flash_puya_chip_erase(void)
{
    uint8_t ret;
    sfc_cmd_t cmd = FLASH_CMD(PUYA_CHIP_ERASE);

    cmd.addr = 0;

    flash_special_write_enable();
    ret = sfc_send_cmd(&cmd);

    flash_special_wait_wip();

    return ret;
}

static uint8_t flash_puya_sw_erase(uint32_t addr)
{
    UNUSED(addr);

    return RET_OK;
}

static uint8_t flash_puya_page_program_sw(uint32_t addr, const void *buf, size_t count)
{
    UNUSED(addr);
    UNUSED(buf);
    UNUSED(count);

    return RET_OK;
}

static uint8_t flash_puya_sw_pe_alg(void)
{
    return RET_OK;
}

static void flash_puya_read_info(void)
{
    uint8_t uid[16];
    uint16_t id;
    uint8_t sfdp;
    uint8_t i;

    if(!(flash_puya_info.v_flag)) {

        id = flash_special_get_id();
        sfdp = flash_special_get_sfdp();
        flash_special_get_uid(uid);
        flash_puya_info.uid |= (((uint32_t)id << 16) | ((uint32_t)sfdp << 8) | (uint32_t)uid[0]);

        for (i = 0; i < ARRAY_SIZE(flash_puya_uid); i++) {
            if (flash_puya_info.uid == flash_puya_uid[i]) {
                flash_puya_info.param_offset = i;

                break;
            }
        }

        flash_puya_info.v_flag = 1;
    }
}

struct flash_ctrl flash_puya_set = {
    .set_quad_mode = flash_puya_set_quad_mode,
    .enable_qpp_mode = flash_puya_enable_qpp_mode,
    .otp_lock = flash_puya_otp_lock,
    .disable_qpp_mode = flash_puya_disable_qpp_mode,
    .page_program = flash_puya_page_program,
    .sector_erase = flash_puya_erase,
    .otp_write = flash_puya_otp_page_program,
    .otp_erase = flash_puya_otp_erase,
    .chip_erase = flash_puya_chip_erase,
    .page_program_sw = flash_puya_page_program_sw,
    .sector_erase_sw = flash_puya_sw_erase,
    .sw_pe_alg = flash_puya_sw_pe_alg,
    .read_info = flash_puya_read_info,
};
