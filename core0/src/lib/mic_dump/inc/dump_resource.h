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
#ifndef MIC_RESOURCE__H_
#define MIC_RESOURCE__H_

#ifdef __cplusplus
extern "C" {
#endif

void dump_set_anc_resource(void);
uint8_t dump_get_asrc_resource(void);
uint8_t dump_get_fifo_resource(void);
uint8_t dump_get_chan_resource(void);

#endif /* MIC_RESOURCE__H_ */
