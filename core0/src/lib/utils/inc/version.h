/****************************************************************************
 *
 Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.
 *
 * This Information is proprietary to WuQi Technologies and MAY NOT
 * be copied by any method or incorporated into another program without
 * the express written consent of WuQi. This Information or any portion
 * thereof remains the property of WuQi. The Information contained herein
 * is believed to be accurate and WuQi assumes no responsibility or
 * liability for its use in any way and conveys no license or title under
 * any patent or copyright and makes no representation or warranty that this
 * Information is free from patent or copyright infringement.
 *
 * ****************************************************************************/

#ifndef LIB_UTILS_VERSION_H
#define LIB_UTILS_VERSION_H

/* The major version 5 bits: 0~31*/
#ifndef FIRMWARE_VERSION_MAJOR
#define FIRMWARE_VERSION_MAJOR (0)
#endif

/* The minor version 7 bits: 0~127*/
#ifndef FIRMWARE_VERSION_MINOR
#define FIRMWARE_VERSION_MINOR (0)
#endif

/* The micro version 4 bits: 0~15*/
#ifndef FIRMWARE_VERSION_MICRO
#define FIRMWARE_VERSION_MICRO (0)
#endif

/* The build version 16 bits: 0~65535*/
#ifndef FIRMWARE_VERSION_BUILD
#define FIRMWARE_VERSION_BUILD (0)
#endif

/* Numerically encoded version */
#define FIRMWARE_VERSION_HEX ((FIRMWARE_VERSION_MAJOR << 27) |  \
                                  (FIRMWARE_VERSION_MINOR << 20) |  \
                                  (FIRMWARE_VERSION_MICRO << 16) |  \
                                  (FIRMWARE_VERSION_BUILD ))

#ifndef FIRMWARE_BUILD_COMMIT
#define FIRMWARE_BUILD_COMMIT "Unknown"
#endif

#ifndef FIRMWARE_BUILD_USER
#define FIRMWARE_BUILD_USER "Anonymous"
#endif

const char* version_get_global_version(void);

#endif /* LIB_UTILS_VERSION_H */
