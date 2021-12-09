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
#ifndef _DRIVER_HW_SFC_H_
#define _DRIVER_HW_SFC_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SFC_ADDR_MASK                 0xFFFFFF
#define SFC_QUAD_IO_FAST_RD_CMD       0xEB
#define SFC_READ_DATA_CMD             0x03
#define SFC_READ_STS_REG1_CMD         0x05
#define SFC_PROGRAM_ERASE_RESUME_CMD  0x7A
#define SFC_PROGRAM_ERASE_SUSPEND_CMD 0x75
#define SFC_QUAD_IO_FAST_RD_CMD_MODE  0x115
#define SFC_QUAD_IO_FAST_RD_CMD_CYCLE 0x08060204
#define SFC_PE_SUSPEND_CMD_CYCLE      0x08000000
#define SFC_READ_DATA_CMD_MODE        0x100
#define SFC_READ_DATA_CMD_CYCLE       0x08180000
#define SFC_READ_STS_REG1_CMD_MODE    0x100
#define SFC_READ_STS_REG1_CMD_CYCLE   0x08000000
#define SFC_PE_RESUME_CMD_MODE        0x100
#define SFC_PE_SUSPEND_CMD_MODE       0x100

typedef enum {
    SFC_OP_PRM,
    SFC_OP_ERASE,
    SFC_OP_TRANS,
    SFC_OP_REG_WR,
} SFC_OPERATION_MODE;

typedef enum {
    SFC_PROG_MIN = 0,
    SFC_PROG_STAND = SFC_PROG_MIN,
    SFC_PROG_QUAD,
    SFC_PROG_FAST,
    SFC_PROG_MAX,
} SFC_PROG_MODE;

typedef enum {
    SFC_SERIAL,
    SFC_DUAL,
    SFC_QUAD,
} SFC_SPI_MODE;

typedef enum {
    SFC_REG_ACCESS,
    SFC_BUF_ACCESS,
} SFC_ACCESS_TYPE;

typedef enum {
    SFC_READ_MIN = 0,
    SFC_READ_BYTE = SFC_READ_MIN,
    SFC_READ_HIGH_SPD,
    SFC_READ_DUAL_FAST,
    SFC_READ_QUAD_FAST,
    SFC_READ_DUAL_IO_FAST,
    SFC_READ_QUAD_IO_FAST,
    SFC_READ_QUAD_IO_WORD_FAST,
    SFC_READ_MAX,
} SFC_READ_MODE;

typedef enum {
    SFC_ENDIAN_BIG = 0,
    SFC_ENDIAN_LITTLE,
}SFC_ENDIAN_MODE;

typedef struct sfc_cmd {
    uint8_t cmd;
    uint32_t addr;
    uint8_t op_mode;
    uint32_t swm_mode;
    uint32_t swm_cycle;
    uint32_t wtime;
} sfc_cmd_t;

typedef struct sfc_pe_param {
    uint16_t manufacturer_id;
    uint8_t sfdp;
    uint16_t resume_wait_time;
    uint16_t sus_wait_time;
    uint16_t pe_cmd_wait_time;
    uint16_t pause_wait_time;
    uint16_t wip_gap_wait_time;
    uint32_t page_program_time;
    uint32_t sector_erase_time;
} sfc_pe_param_t;

void sfc_init(void);
uint8_t sfc_send_cmd(const sfc_cmd_t *cmd);
uint8_t sfc_send_read_cmd(const sfc_cmd_t *cmd, uint8_t *data, uint32_t data_len);
uint8_t sfc_send_write_cmd(const sfc_cmd_t *cmd, const uint8_t *data, uint32_t data_len);

void sfc_set_cache_mode(void);
void sfc_set_io_map(uint32_t map);
void sfc_set_edge(uint8_t rx, uint8_t tx);
void sfc_set_endian_mode(SFC_ENDIAN_MODE endian);
void sfc_set_suspend_resume_param(void);
uint32_t sfc_get_io_map(void);
uint8_t sfc_get_wip_timeout_flag(void);
uint8_t sfc_send_soft_reset_cmd_with_pe_sm(void);
bool_t sfc_is_pe_in_progress(void);
void sfc_set_dynamic_suspend_resume_param(const sfc_pe_param_t *pe_param, uint32_t wtime);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_SFC_H_ */
