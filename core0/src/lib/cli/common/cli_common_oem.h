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

#ifndef __CLI_COMMON_OEM_H_
#define __CLI_COMMON_OEM_H_

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief write ppm to oem
 *
 * @param buffer is cli command payload.
 * @param bufferlen is cli command payload length.
 */
void cli_write_oem_ppm_handler(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief write mac to oem
 *
 * @param buffer is cli command payload.
 * @param bufferlen is cli command payload length.
 */
void cli_write_oem_mac_handler(uint8_t *buffer, uint32_t bufferlen);

#ifdef __cplusplus
}
#endif

#endif
