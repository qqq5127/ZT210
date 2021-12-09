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

#ifndef LIB_CLI_H
#define LIB_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cli_command.h"
#include "generic_transmission_api.h"
#include "generic_transmission_config.h"

#define DEFAULT_CLI_TASK_SIZE   256
#define DEFAULT_CLI_TASK_PRIO 8
#define DEFAULT_CLI_MAX_BUFFER_LEN 512
#define DBGLOG_LIB_CLI_RAW(fmt, arg...)      DBGLOG_LOG(IOT_CLI_MID, DBGLOG_LEVEL_VERBOSE, fmt, ##arg)
#define DBGLOG_LIB_CLI_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(IOT_CLI_MID, fmt, ##arg)

typedef struct cli_config {
    size_t cli_task_size;
    uint8_t cli_prio;
} cli_config_t;

/**
 * @brief This function is used to init cli.
 *
 * @return uint8_t RET_OK.
 */
uint8_t cli_init(cli_config_t *cli_config);

/**
 * @brief This function is used to deinit cli process.
 *
 * @return uint8_t RET_OK.
 */
uint8_t cli_deinit(void);

/**
 * @brief This function is used to process cli meaasge.
 *        the buffer should be an complete cli command,
 *        contains cli head and cli payload
 *
 * @return uint8_t RET_OK.
 */
uint8_t cli_interface_msg_receive(generic_transmission_tid_t tid,generic_transmission_data_type_t type,
                                  uint8_t *buffer, uint16_t len,
                                  generic_transmission_data_rx_cb_st_t status);

/**
 * @brief This function is used to set response message for cli.
 *
 * @param module_id is the module id of function.
 * @param msg_id is the message id of function.
 * @param buffer required by function.
 * @param buffer_len required by function.
 * @param sn is message sn. (deprecated)
 * @param result is message result.
 * @return uint8_t RET_OK.
 */
uint8_t cli_interface_msg_response(uint32_t module_id, uint32_t msg_id,
                                   uint8_t *buffer, uint32_t buffer_len,
                                   uint16_t sn, uint8_t result);

/**
 * @brief This function is used to set ind message for cli.
 *
 * @param module_id is the module id of function.
 * @param msg_id is the message id of function.
 * @param buffer required by function.
 * @param buffer_len required by function.
 * @param sn is message sn. (deprecated)
 * @param result is message result.
 * @return uint8_t RET_OK.
 */
uint8_t cli_interface_msg_ind(uint32_t module_id, uint32_t msg_id,
                                   uint8_t *buffer, uint32_t buffer_len,
                                   uint16_t sn, uint8_t result);

/**
 * @brief This function is used to get current cmd sn.
 *
 * @return uint16_t current cmd sn.
 */
uint16_t cli_interface_get_current_cmd_sn(void);

#ifdef __cplusplus
}
#endif

#endif /* LIB_CLI_H */
