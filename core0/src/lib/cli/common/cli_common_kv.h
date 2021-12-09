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

#ifndef LIB_CLI_COMMON_KV_H
#define LIB_CLI_COMMON_KV_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set kv item.
 *
 * @param buffer Cli command payload
 * @param bufferlen Cli command payload length
 */
void cli_common_basic_set_kv(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief get kv item.
 *
 * @param buffer Cli command payload
 * @param bufferlen Cli command payload length
 */
void cli_common_basic_get_kv(uint8_t *buffer, uint32_t bufferlen);
#ifdef __cplusplus
}
#endif

#endif /* LIB_CLI_COMMON_KV_H */
