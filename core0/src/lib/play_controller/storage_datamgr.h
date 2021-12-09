
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

#ifndef _SRC_APP_TWS_APP_STORAGE_DATAMGR_H_
#define _SRC_APP_TWS_APP_STORAGE_DATAMGR_H_

#include "types.h"
#include "storage_controller.h"

/*lint (AUDIO_CONTROL_CFG_ID_MAX) not referenced */
enum {
    AUDIO_GAIN_DRC_CFG_ID = AUDIO_CONTROL_BASE_ID,
    AUDIO_ANC_COEFF_IDX_CFG_ID,
    AUDIO_BT_CFG_IDX_CFG_ID,
    AUDIO_GAIN_OFFSET_CFG_ID,
    AUDIO_CONTROL_CFG_ID_MAX = AUDIO_CONTROL_END_ID,
};


#endif /* _SRC_APP_TWS_APP_STORAGE_DATAMGR_H_ */
