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

#ifndef _SRC_LIB_PLAYER_DATAMGR_CORES_INC_NPLAYER_DATAMGR_ITL_H_
#define _SRC_LIB_PLAYER_DATAMGR_CORES_INC_NPLAYER_DATAMGR_ITL_H_
#ifdef NEW_ARCH
#include "types.h"
#include "modules.h"
#include "nplayer_datamgr_core.h"

/**
 * @brief player_datamgr_init
 *
 * @param[in] p_cfg       The pointer of btcore config.
 *
 * @return void.
 */
void player_datamgr_init(void *p_cfg);
#endif // NEW_ARCH
#endif /* _SRC_LIB_PLAYER_DATAMGR_CORES_INC_NPLAYER_DATAMGR_ITL_H_ */
