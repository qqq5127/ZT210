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

#ifndef _DRIVER_HW_MAILBOX_H
#define _DRIVER_HW_MAILBOX_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAILBOX_CH_MAX         8
#define MAILBOX_DIVI_CHANNEL   6
#define MAILBOX_DATA_FIFO_SIZE 64

typedef enum {
    MAILBOX_ID_0 = 0,
    MAILBOX_ID_1 = 1,
} MAILBOX_ID;

void mailbox_init(uint8_t mailbox_id);
uint32_t mailbox_get_read_space(uint8_t ch);
uint32_t mailbox_get_write_space(uint8_t ch);
uint32_t mailbox_read(uint8_t ch, uint32_t *p_data);
uint32_t mailbox_write(uint8_t ch, uint32_t data);
uint32_t mailbox_get_irq_vector(uint8_t ch);
uint32_t mailbox_recvint_clean(uint8_t ch);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_MAILBOX_H */
