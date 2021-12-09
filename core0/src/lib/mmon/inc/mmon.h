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
#ifndef LIB_MMON_H
#define LIB_MMON_H

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup MMON
 * @{
 * This section introduces the memory monitor module's enum, structure, functions and how to use this module.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is to enable memory range access.
 *
 * @param start is memeory start address.
 * @param size  is memory size.
 * @return bool_t is true if pmp configure successfully.
 */
bool_t mmon_disable_address_access(uint32_t start, uint32_t size);

/**
 * @brief This function is to disable memory range access.
 *
 * @param start is memeory start address.
 * @param size  is memory size.
 * @return bool_t is true if pmp configure successfully.
 */
bool_t mmon_enable_address_access(uint32_t start, uint32_t size);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup MMON
 */

/**
* @}
 * addtogroup LIB
*/

#endif   //LIB_MMON_H
