/****************************************************************************

Copyright(c) 2021 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
#ifdef NEW_ARCH

#ifndef _SRC_LIB_PLAYER_SYNC_CORES_INC_NPLAYER_SYNC_ITL_H_
#define _SRC_LIB_PLAYER_SYNC_CORES_INC_NPLAYER_SYNC_ITL_H_
/*
 * INCLUDE FILES
 ****************************************************************************
 */
#include "types.h"
#include "modules.h"

/*
 * MACROS
 ****************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************
 */

/*
 * ENUMERATIONS
 ****************************************************************************
 */

/*
 * TYPE DEFINITIONS
 ****************************************************************************
 */

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************
 */

void player_config_frequence(uint32_t freq_in, uint32_t freq_out, int16_t ppm);

#endif /* _SRC_LIB_PLAYER_SYNC_CORES_INC_NPLAYER_SYNC_ITL_H_ */
#endif // NEW_PLAYER_ARCH

