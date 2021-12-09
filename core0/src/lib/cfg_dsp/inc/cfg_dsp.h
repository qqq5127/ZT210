
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

#ifndef _CFG_DSP_H_
#define _CFG_DSP_H_

#include "types.h"

/**
 * @brief entry for cfg dsp param
 *
 * @return 0 for success, else for the error code
 */
uint32_t cfg_dsp_entry(void);

/**
 * @brief get dsp voice algrithm module enable state
 *
 * @return Please check enum voice_param_type_t to find
 * out the detail bit stands for which module.For each bit,
 * 0 means the corresponding module is disabled,and 1 means
 * the corresponding module is enabled.
 */
uint32_t get_dsp_voice_enable_state(void);

#endif /* _CFG_DSP_H_ */
