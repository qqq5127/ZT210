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
#include "version.h"

#define VERSION_PREFIX_STR "FW VERSION "
#define VERSION_NUM_STR                                                      \
    STR(FIRMWARE_VERSION_MAJOR)                                              \
    "." STR(FIRMWARE_VERSION_MINOR) "." STR(FIRMWARE_VERSION_MICRO) "." STR( \
        FIRMWARE_VERSION_BUILD)

#ifdef BUILD_FROM_SDK
#define VERSION_SUFFIX_STR "-SDK"
#else
#define VERSION_SUFFIX_STR ""
#endif

#if defined(RELEASE)
#define VERSION_BUILD_STR "Release"
#elif defined(DEVELOPMENT)
#define VERSION_BUILD_STR "Development"
#else
#define VERSION_BUILD_STR "Debug"
#endif

#ifndef VERSION_EXTRA_STR
static const char *const global_version_str =
    VERSION_PREFIX_STR "-" VERSION_NUM_STR "-" VERSION_BUILD_STR VERSION_SUFFIX_STR;
#else
static const char *const global_version_str = VERSION_PREFIX_STR
    "-" VERSION_NUM_STR "-" VERSION_BUILD_STR VERSION_SUFFIX_STR "-" VERSION_EXTRA_STR;
#endif

const char *version_get_global_version(void)
{
    return global_version_str;
}
