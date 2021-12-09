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

#ifndef ERRNO_H
#define ERRNO_H

/**
 * @addtogroup UTILS
 * @{
 * @addtogroup ERRNO
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum RET_TYPE
 * @brief generic return type
 */
typedef enum {
    RET_OK,             /**< successful */
    RET_INVAL,          /**< invalid parameters */
    RET_NOMEM,          /**< out of memory */
    RET_NOSUPP,         /**< not supported */
    RET_NOSEC_WL,       /**< not secure due to white list */
    RET_NOT_EXIST,      /**< not exist */
    RET_AGAIN,          /**< again */
    RET_NOT_READY,      /**< dev not ready */
    RET_EXIST,          /**< already exist */
    RET_BUSY,           /**< busy */
    RET_PENDING,        /**< pending */
    RET_FAIL,           /**< failed */
    RET_NOSEC_BL,       /**< not secure due to black list */
    RET_CRC_LEN,        /**< calculated crc but len < 0 */
    RET_DISCONNECT,     /**< disconnect */
    RET_TIMEOVER,       /**< timeout */
    RET_CRC_FAIL,       /**< crc check failed */
} RET_TYPE;

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup ERRNO
 * @}
 * addtogroup UTILS
 */

#endif /* ERRNO_H */
