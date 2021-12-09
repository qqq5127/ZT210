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
#ifndef LIB_UTILS_B64_H
#define LIB_UTILS_B64_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is to encode a string array.
 *
 * @param src is src string.
 * @param src_len is src len
 * @param dst is dest string
 */
void b64_encode(const uint8_t *src, uint32_t src_len, uint8_t *dst);

#ifdef __cplusplus
}
#endif

#endif /* LIB_UTILS_B64_H */
